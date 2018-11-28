#include "hal.hh"
#include "codec.hh"
#include "system.hh"
#include "debug.hh"
#include "buttons.hh"
#include "gates.hh"
#include "switches.hh"
#include "leds.hh"
#include "adc.hh"
#include "polyptic_oscillator.hh"

bool do_audio_passthrough_test = false;


#define MAX_CODEC_DAC_VAL 8388607
#define MIN_CODEC_DAC_VAL (-8388608)

int32_t average_L, average_R;

int32_t tri_L=0, tri_R=0;
int32_t tri_L_dir=1, tri_R_dir=1;

struct Main : Nocopy {
  System sys_;
  Buttons buttons_;
  Gates gates_;
  Switches switches_;
  Leds leds_;
  Adc adc_;
  Codec codec_{kSampleRate,
               [this](Frame* in, Frame *out, int size) {
                 Process(in, out, size);
               }};

  PolypticOscillator osc_;

  // UI state variables
  bool freeze_jack, learn_jack;
  bool learn_but, freeze_but;
  Switches::State 	mod_sw, grid_sw, twist_sw, warp_sw;
  u0_16 warp_pot, detune_pot, mod_pot, root_pot, grid_pot, pitch_pot, spread_pot, tilt_pot, twist_pot;
  u0_16 spread1_cv, warp_cv, spread2_cv, twist_cv, tilt_cv, grid_cv, mod_cv;

  // other state variables
  uint32_t learn_but_armed=0, freeze_but_armed=0;
  uint8_t learn_color=1, freeze_color=1;
  uint32_t last_update_tm = 0;

  Main() {
    //Start audio processing
    codec_.Start();

    // process
    while(1) {
      Process();
    }
  }

private:

  void Process() {

    //LEDs with PWM
    if ((HAL_GetTick() - last_update_tm) > 1000/60) {
      last_update_tm = HAL_GetTick();
      u0_8 ledpwm[6];
      ledpwm[0] = u0_8::narrow(adc_.get_adc(Adc::GRID_POT));
      ledpwm[1] = u0_8::narrow(adc_.get_adc(Adc::SPREAD_POT));
      ledpwm[2] = u0_8::narrow(adc_.get_adc(Adc::PITCH_POT));
      ledpwm[3] = u0_8::narrow(adc_.get_adc(Adc::TILT_CV));
      ledpwm[4] = u0_8::narrow(adc_.get_adc(Adc::GRID_CV));
      ledpwm[5] = u0_8::narrow(adc_.get_adc(Adc::SPREAD_CV_1));
      leds_.set_freeze(ledpwm[0], ledpwm[1], ledpwm[2]);
      leds_.set_learn(ledpwm[3], ledpwm[4], ledpwm[5]);
    }

    ///////////////////////////////////////////////////////////////////////////////////////
    // Run the code at normal speed and watch the variables in the debugger. 
    // The appropriate variables should respond to incoming gates/CV and turning the pots
    ///////////////////////////////////////////////////////////////////////////////////////

    //Gate jacks
    freeze_jack = gates_.freeze_.get();
    learn_jack = gates_.learn_.get();

    //Buttons
    freeze_but = buttons_.freeze_.get();
    learn_but = buttons_.learn_.get();

    // Switches
    do_audio_passthrough_test = switches_.mod_.get() == Switches::DOWN;

    //Switches
    mod_sw = switches_.mod_.get();
    grid_sw = switches_.grid_.get();
    twist_sw = switches_.twist_.get();
    warp_sw = switches_.warp_.get();

    //ADCs
    warp_pot = adc_.get_adc(Adc::WARP_POT);
    detune_pot = adc_.get_adc(Adc::DETUNE_POT);
    mod_pot = adc_.get_adc(Adc::MOD_POT);
    root_pot = adc_.get_adc(Adc::ROOT_POT);
    grid_pot = adc_.get_adc(Adc::GRID_POT);
    pitch_pot = adc_.get_adc(Adc::PITCH_POT);
    spread_pot = adc_.get_adc(Adc::SPREAD_POT);
    tilt_pot = adc_.get_adc(Adc::TILT_POT);
    twist_pot = adc_.get_adc(Adc::TWIST_POT);

    spread1_cv = adc_.get_adc(Adc::SPREAD_CV_1);
    warp_cv = adc_.get_adc(Adc::WARP_CV);
    spread2_cv = adc_.get_adc(Adc::SPREAD_CV_2);
    twist_cv = adc_.get_adc(Adc::TWIST_CV);
    tilt_cv = adc_.get_adc(Adc::TILT_CV);
    grid_cv = adc_.get_adc(Adc::GRID_CV);
    mod_cv = adc_.get_adc(Adc::MOD_CV);
  }


  void Process(Frame *in, Frame *out, int size) {
    if (do_audio_passthrough_test) {
      while(size--) {
        *out = *in;
        out++;
        in++;
      }
    } else {
      f freq = 0.01_f;
      Float o[size];
      osc_.Process(freq, o, size);

      Float *p = o;
      while(size--) {
        s1_15 s = s1_15(*p);
        out->l = s.repr();
        out->r = s.repr();
        out++; p++;
      }
    }
  }
} _;
