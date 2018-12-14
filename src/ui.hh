#include "buttons.hh"
#include "gates.hh"
#include "switches.hh"
#include "leds.hh"
#include "adc.hh"

const int kPotFiltering = 1;     // 0..16
const f kPotDeadZone = 0.01_f;

f test;

// law + one-pole filter + crop ends -> float(0..1)
// TODO one-pole --> nonlinear square

class Control {

  enum Law { LINEAR, QUADRATIC, CUBIC, QUARTIC };

  template<Law LAW, int LP>  // Lp = 0..16
  class PotConditioner {
    QuadraticOnePoleLp<LP> lp_;
  public:
    s1_15 Process(u0_16 in) {
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

public:
  void Process(Parameters &params) {

    s1_15 d = detune_pot.Process(adc_.get_adc(Adc::DETUNE_POT));
    f detune = Math::crop_down(kPotDeadZone, d.to_float());
    detune = (detune * detune) * (detune * detune);
    params.detune = detune;

    s1_15 t = tilt_pot.Process(adc_.get_adc(Adc::TILT_POT));
    f tilt = Math::crop(kPotDeadZone, t.to_float());
    tilt = tilt * 2_f - 1_f;
    tilt *= tilt * tilt;
    tilt *= 10_f;
    tilt = Math::fast_exp2(tilt);
    params.tilt = tilt;

    s1_15 warp = warp_pot.Process(adc_.get_adc(Adc::WARP_POT));
    params.warp.value = warp.to_float();

    s1_15 w = twist_pot.Process(adc_.get_adc(Adc::TWIST_POT));
    f twist = w.to_float();

    if (params.twist.mode == FEEDBACK) {
      twist *= twist;
    } else if (params.twist.mode == PULSAR) {
      twist = 1_f - twist;
      twist *= twist;
    } else if (params.twist.mode == DECIMATE) {
      twist *= twist * 0.5_f;
    }

    params.twist.value = twist;

    s1_15 root = root_pot.Process(adc_.get_adc(Adc::ROOT_POT));
    params.root = root.to_float();

    s1_15 pitch = pitch_pot.Process(adc_.get_adc(Adc::PITCH_POT));
    params.pitch = pitch.to_float() * 12_f * 8_f + 24_f;
    
    s1_15 s = spread_pot.Process(adc_.get_adc(Adc::SPREAD_POT));
    f spread = Math::crop_down(kPotDeadZone, s.to_float());
    spread *= spread;
    params.spread = spread * 12_f;
    


    // warp_pot = adc_.get_adc(Adc::WARP_POT);
    // detune_pot = adc_.get_adc(Adc::DETUNE_POT);
    // mod_pot = adc_.get_adc(Adc::MOD_POT);
    // root_pot = adc_.get_adc(Adc::ROOT_POT);
    // grid_pot = adc_.get_adc(Adc::GRID_POT);
    // pitch_pot = adc_.get_adc(Adc::PITCH_POT);
    // spread_pot = adc_.get_adc(Adc::SPREAD_POT);
    // tilt_pot = adc_.get_adc(Adc::TILT_POT);
    // twist_pot = adc_.get_adc(Adc::TWIST_POT);

    // spread1_cv = adc_.get_adc(Adc::SPREAD_CV_1);
    // warp_cv = adc_.get_adc(Adc::WARP_CV);
    // spread2_cv = adc_.get_adc(Adc::SPREAD_CV_2);
    // twist_cv = adc_.get_adc(Adc::TWIST_CV);
    // tilt_cv = adc_.get_adc(Adc::TILT_CV);
    // grid_cv = adc_.get_adc(Adc::GRID_CV);
    // mod_cv = adc_.get_adc(Adc::MOD_CV);
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
  Ui() {
  }

  void Process(Parameters& params) {

    control_.Process(params);

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
