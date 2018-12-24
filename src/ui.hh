#include "buttons.hh"
#include "gates.hh"
#include "switches.hh"
#include "leds.hh"
#include "adc.hh"


const int kPotFiltering = 1;     // 0..16
const f kPotDeadZone = 0.01_f;

class Control {

  enum Law { LINEAR, QUADRATIC, CUBIC, QUARTIC };

  template<Law LAW, int LP>  // Lp = 0..16
  class PotConditioner {
    QuadraticOnePoleLp<LP> lp_;
  public:
    s1_15 Process(Adc::Channel ch) {
      u0_16 in = ch.get();
      switch(LAW) {
      case LINEAR: break;
      case QUADRATIC: in = u0_16::narrow(in * in); break;
      case CUBIC: in = u0_16::narrow(u0_16::narrow(in * in) * in); break;
      case QUARTIC:
        in = u0_16::narrow(in * in);
        in = u0_16::narrow(in * in);
        break;
      }
      s1_15 x = in.to_signed();
      x = lp_.Process(x);
      return x;
    }
  };

  struct CVConditioner {
    s1_15 Process(Adc::Channel ch) {
      s1_15 x = ch.get().to_signed();
      return x;
    }
  };

  class AudioCVConditioner {
    CicDecimator<1, kBlockSize> cic_;
  public:
    f Process(Block<s1_15> in) {
      s1_15 x = in[0];
      cic_.Process(in.begin(), &x, 1);
      return x.to_float_inclusive();
    }
  };

  Adc adc_;

  PotConditioner<LINEAR, kPotFiltering> warp_pot;
  PotConditioner<LINEAR, kPotFiltering> detune_pot;
  PotConditioner<LINEAR, kPotFiltering> mod_pot;
  PotConditioner<LINEAR, kPotFiltering> root_pot;
  PotConditioner<LINEAR, kPotFiltering> grid_pot;
  PotConditioner<LINEAR, kPotFiltering> pitch_pot;
  PotConditioner<LINEAR, kPotFiltering> spread_pot;
  PotConditioner<LINEAR, kPotFiltering> tilt_pot;
  PotConditioner<LINEAR, kPotFiltering> twist_pot;

  CVConditioner warp_cv;
  CVConditioner spread_cv;
  CVConditioner twist_cv;
  CVConditioner tilt_cv;
  CVConditioner grid_cv;
  CVConditioner mod_cv;

  AudioCVConditioner pitch_cv;
  AudioCVConditioner root_cv;

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

    f p_cv = pitch_cv.Process(Block<s1_15> {in1, size});
    f r_cv = root_cv.Process(Block<s1_15> {in2, size});

    p_cv = 0_f;                 // TODO
    r_cv = 0_f;                 // TODO

    // Process potentiometer & CV

    s1_15 d = detune_pot.Process(adc_.detune_pot);
    f detune = Math::crop_down(kPotDeadZone, d.to_float_inclusive());
    detune = (detune * detune) * (detune * detune);
    params.detune = detune;

    s1_15 t = tilt_pot.Process(adc_.tilt_pot);
    t += tilt_cv.Process(adc_.tilt_cv);
    f tilt = Math::crop(kPotDeadZone, t.to_float_inclusive());
    tilt = tilt * 2_f - 1_f;
    tilt *= tilt.abs();
    tilt *= 8_f;
    tilt = Math::fast_exp2(tilt);
    params.tilt = tilt;

    s1_15 warp = warp_pot.Process(adc_.warp_pot);
    params.warp.value = warp.to_float_inclusive();

    s1_15 w = twist_pot.Process(adc_.twist_pot);
    f twist = w.to_float_inclusive();

    if (params.twist.mode == FEEDBACK) {
      twist *= twist;
    } else if (params.twist.mode == PULSAR) {
      twist = 1_f - twist;
      twist *= twist;
    } else if (params.twist.mode == DECIMATE) {
      twist *= twist * 0.5_f;
    }

    params.twist.value = twist;

    s1_15 root = root_pot.Process(adc_.root_pot);
    params.root = root.to_float_inclusive() * 12_f * 10_f;
    params.root += r_cv * 12_f * 4_f;

    s1_15 pitch = pitch_pot.Process(adc_.pitch_pot);
    params.pitch = pitch.to_float_inclusive() * 12_f * 6_f - 24_f;
    params.pitch += p_cv * 12_f * 4_f;
    
    s1_15 s = spread_pot.Process(adc_.spread_pot);
    f spread = Math::crop_down(kPotDeadZone, s.to_float_inclusive());
    spread *= spread;
    params.spread = spread * 12_f;

    // warp_pot = adc_.get(Adc::WARP_POT);
    // detune_pot = adc_.get(Adc::DETUNE_POT);
    // mod_pot = adc_.get(Adc::MOD_POT);
    // root_pot = adc_.get(Adc::ROOT_POT);
    // grid_pot = adc_.get(Adc::GRID_POT);
    // pitch_pot = adc_.get(Adc::PITCH_POT);
    // spread_pot = adc_.get(Adc::SPREAD_POT);
    // tilt_pot = adc_.get(Adc::TILT_POT);
    // twist_pot = adc_.get(Adc::TWIST_POT);

    // spread1_cv = adc_.get(Adc::SPREAD_CV_1);
    // warp_cv = adc_.get(Adc::WARP_CV);
    // spread2_cv = adc_.get(Adc::SPREAD_CV_2);
    // twist_cv = adc_.get(Adc::TWIST_CV);
    // tilt_cv = adc_.get(Adc::TILT_CV);
    // grid_cv = adc_.get(Adc::GRID_CV);
    // mod_cv = adc_.get(Adc::MOD_CV);

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
