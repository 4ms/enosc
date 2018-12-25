#include "buttons.hh"
#include "gates.hh"
#include "switches.hh"
#include "leds.hh"
#include "adc.hh"


const int kPotFiltering = 1;     // 0..16
const f kPotDeadZone = 0.01_f;

class Control {

  enum Law { LINEAR, QUADRATIC, CUBIC, QUARTIC };

  template<Law LAW>  // Lp = 0..16
  class PotConditioner {
  public:
    u0_16 Process(u0_16 x) {
      switch(LAW) {
      case LINEAR: break;
      case QUADRATIC: x = u0_16::narrow(x * x); break;
      case CUBIC: x = u0_16::narrow(u0_16::narrow(x * x) * x); break;
      case QUARTIC:
        x = u0_16::narrow(x * x);
        x = u0_16::narrow(x * x);
        break;
      }
      return x;
    }
  };

  struct CVConditioner {
    s1_15 Process(u0_16 in) {
      // TODO calibration
      s1_15 x = in.to_signed_scale();
      return x;                 // -1..1
    }
  };

  struct None { s1_15 Process(u0_16) { return 0._s1_15; } };

  template<class PotConditioner, class CVConditioner, int LP>
  struct PotCVCombiner {
    PotConditioner pot_;
    CVConditioner cv_;
    QuadraticOnePoleLp<LP> lp_;
    f Process(u0_16 pot, u0_16 cv) {
      s17_15 x = s17_15(pot_.Process(pot).to_signed());
      // TODO use saturating add
      x -= s17_15(cv_.Process(cv));
      f y = x.to_float_inclusive().clip(0_f, 1.0_f);
      return lp_.Process(y);
    }
    f Process(u0_16 pot) {
      f x = pot_.Process(pot).to_float_inclusive();
      return lp_.Process(x);
    }
  };

  class AudioCVConditioner {
    QuadraticOnePoleLp<2> lp_;
    CicDecimator<1, kBlockSize> cic_;
  public:
    f Process(Block<s1_15> in) {
      s1_15 x = in[0];
      cic_.Process(in.begin(), &x, 1);
      return lp_.Process(x.to_float_inclusive());
    }
  };

  Adc adc_;

  PotCVCombiner<PotConditioner<LINEAR>, None, kPotFiltering> detune_;
  PotCVCombiner<PotConditioner<LINEAR>, CVConditioner, kPotFiltering> warp_;
  PotCVCombiner<PotConditioner<LINEAR>, CVConditioner, kPotFiltering> tilt_;
  PotCVCombiner<PotConditioner<LINEAR>, CVConditioner, kPotFiltering> twist_;
  PotCVCombiner<PotConditioner<LINEAR>, CVConditioner, kPotFiltering> grid_;
  PotCVCombiner<PotConditioner<LINEAR>, CVConditioner, kPotFiltering> mod_;
  PotCVCombiner<PotConditioner<LINEAR>, CVConditioner, kPotFiltering> spread_;

  PotConditioner<LINEAR> pitch_pot_;
  PotConditioner<LINEAR> root_pot_;
  AudioCVConditioner pitch_cv_;
  AudioCVConditioner root_cv_;
  QuadraticOnePoleLp<2> root_pot_lp_;
  QuadraticOnePoleLp<2> pitch_pot_lp_;

public:
  void Process(Block<Frame> codec_in, Parameters &params) {

    // Process codec input
    int size = codec_in.size();
    s1_15 in1[size], in2[size];
    Block<s1_15> pitch_block {in1, size};
    Block<s1_15> root_block {in2, size};

    auto *pi=pitch_block.begin(), *ro=root_block.begin();
    for (Frame in : codec_in) {
      *pi++ = in.l;
      *ro++ = in.r;
    }

    // Process potentiometer & CV

    f detune = detune_.Process(adc_.detune_pot());
    detune = Math::crop_down(kPotDeadZone, detune);
    detune = (detune * detune) * (detune * detune);
    params.detune = detune;

    f tilt = tilt_.Process(adc_.tilt_pot(), adc_.tilt_cv());
    tilt = Math::crop(kPotDeadZone, tilt);
    tilt = tilt * 2_f - 1_f;
    tilt *= tilt.abs();
    tilt *= 8_f;
    tilt = Math::fast_exp2(tilt);
    params.tilt = tilt;

    f warp = warp_.Process(adc_.warp_pot(), adc_.warp_cv());
    warp = Math::crop(kPotDeadZone, warp);
    params.warp.value = warp;

    f twist = twist_.Process(adc_.twist_pot(), adc_.twist_cv());
    twist = Math::crop(kPotDeadZone, twist);
    if (params.twist.mode == FEEDBACK) {
      twist *= twist;
    } else if (params.twist.mode == PULSAR) {
      twist = 1_f - twist;
      twist *= twist;
    } else if (params.twist.mode == DECIMATE) {
      twist *= twist * 0.5_f;
    }
    params.twist.value = twist;

    f spread = spread_.Process(adc_.spread_pot(), adc_.spread_cv());
    spread = Math::crop(kPotDeadZone, spread);
    spread *= spread;
    params.spread = spread * 12_f;

    // Root & Pitch
    u0_16 r = root_pot_.Process(adc_.root_pot());
    f root = root_pot_lp_.Process(r.to_float_inclusive());
    root *= 12_f * 10_f; // 0..120
    f root_cv = root_cv_.Process(root_block);  // -1..1
    root += root_cv * 12_f * 4_f;
    params.root = root;

    u0_16 p = pitch_pot_.Process(adc_.pitch_pot());
    f pitch = pitch_pot_lp_.Process(p.to_float_inclusive());
    pitch = pitch * 12_f * 6_f - 24_f;
    f pitch_cv = pitch_cv_.Process(pitch_block); // -1..1
    pitch += pitch_cv * 12_f * 4_f;
    params.pitch = pitch;
    
    // Start next conversion
    adc_.Start();
  }
};

class Ui {
  Buttons buttons_;
  Gates gates_;
  Switches switches_;
  Leds leds_;
  Control control_;

  // UI state variables
  bool freeze_jack, learn_jack;
  bool learn_but, freeze_but;
  Switches::State mod_sw, grid_sw, twist_sw, warp_sw;

public:
  void Process(Block<Frame> codec_in, Parameters& params) {

    control_.Process(codec_in, params);

    //Gate jacks
    freeze_jack = gates_.freeze_.get();
    learn_jack = gates_.learn_.get();

    //Buttons
    freeze_but = buttons_.freeze_.get();
    learn_but = buttons_.learn_.get();

    //Switches
    mod_sw = switches_.mod_.get();
    grid_sw = switches_.grid_.get();
    twist_sw = switches_.twist_.get();

    params.twist.mode = static_cast<TwistMode>(switches_.twist_.get());
    params.warp.mode = static_cast<WarpMode>(switches_.warp_.get());

    params.stereo_mode = static_cast<StereoMode>(switches_.mod_.get());
  }

};
