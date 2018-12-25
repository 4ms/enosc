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
    u0_16 Process(Adc::Channel ch) {
      u0_16 x = ch.get();
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
    s1_15 Process(Adc::Channel ch) {
      // TODO calibration
      s1_15 x = ch.get().to_signed_scale();
      return x;                 // -1..1
    }
  };

  struct None { s1_15 Process(Adc::Channel) { return 0._s1_15; } };

  template<class PotConditioner, class CVConditioner, int LP>
  struct PotCVCombiner {
    PotConditioner pot_;
    CVConditioner cv_;
    QuadraticOnePoleLp<LP> lp_;
    f Process(Adc::Channel pot, Adc::Channel cv) {
      s1_15 x = pot_.Process(pot).to_signed();
      x = x.sub_sat(cv_.Process(cv));
      if (x<0._s1_15) x=0._s1_15; // TODO use saturating add
      return lp_.Process(x.to_float_inclusive());
    }
    f Process(Adc::Channel pot) {
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

  // TODO encapsuler
  PotConditioner<LINEAR> root_pot;
  PotConditioner<LINEAR> pitch_pot;
  AudioCVConditioner pitch_cv;
  AudioCVConditioner root_cv;
  QuadraticOnePoleLp<2> root_pot_lp_;
  QuadraticOnePoleLp<2> pitch_pot_lp_;
  QuadraticOnePoleLp<2> root_cv_lp_;
  QuadraticOnePoleLp<2> pitch_cv_lp_;

public:
  void Process(Block<Frame> codec_in, Parameters &params) {

    // Process codec input
    int size = codec_in.size();
    s1_15 in1[size];
    s1_15 in2[size];

    s1_15 *i1=in1, *i2=in2;
    for (Frame in : codec_in) {
      *i1++ = in.l;
      *i2++ = in.r;
    }

    // Root & Pitch
    f p_cv = pitch_cv.Process(Block<s1_15> {in1, size}); // -1..1
    f r_cv = root_cv.Process(Block<s1_15> {in2, size});  // -1..1

    // Process potentiometer & CV

    f detune = detune_.Process(adc_.detune_pot);
    detune = Math::crop_down(kPotDeadZone, detune);
    detune = (detune * detune) * (detune * detune);
    params.detune = detune;

    f tilt = tilt_.Process(adc_.tilt_pot, adc_.tilt_cv);
    tilt = Math::crop(kPotDeadZone, tilt);
    tilt = tilt * 2_f - 1_f;
    tilt *= tilt.abs();
    tilt *= 8_f;
    tilt = Math::fast_exp2(tilt);
    params.tilt = tilt;

    params.warp.value = warp_.Process(adc_.warp_pot);

    f twist = twist_.Process(adc_.twist_pot, adc_.twist_cv);
    if (params.twist.mode == FEEDBACK) {
      twist *= twist;
    } else if (params.twist.mode == PULSAR) {
      twist = 1_f - twist;
      twist *= twist;
    } else if (params.twist.mode == DECIMATE) {
      twist *= twist * 0.5_f;
    }
    params.twist.value = twist;

    // TODO encapsuer ds classe
    u0_16 r = root_pot.Process(adc_.root_pot);
    f root = root_pot_lp_.Process(r.to_float_inclusive());
    root *= 12_f * 10_f; // 0..120
    root += root_cv_lp_.Process(r_cv) * 12_f * 4_f;
    params.root = root;

    u0_16 p = pitch_pot.Process(adc_.pitch_pot);
    f pitch = pitch_pot_lp_.Process(p.to_float_inclusive());
    pitch = pitch * 12_f * 6_f - 24_f;
    pitch += pitch_cv_lp_.Process(p_cv) * 12_f * 4_f;
    params.pitch = pitch;
    
    f spread = spread_.Process(adc_.spread_pot);
    spread *= spread;
    params.spread = spread * 12_f;

    // 
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
