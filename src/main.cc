#include "hal.hh"
#include "codec.hh"
#include "system.hh"
#include "buttons.hh"
#include "gates.hh"
#include "switches.hh"
#include "debug.hh"
#include "leds.hh"
#include "adc.hh"

bool do_audio_passthrough_test = false;


#define MAX_CODEC_DAC_VAL 8388607
#define MIN_CODEC_DAC_VAL (-8388608)

int32_t average_L, average_R;

int32_t tri_L=0, tri_R=0;
int32_t tri_L_dir=1, tri_R_dir=1;

struct Main : Codec::Callback {
  System sys_;
  Buttons buttons_;
  Gates gates_;
  Switches switches_;
  Leds leds_;
  Adc adc_;
  Codec codec_{this, kSampleRate};

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
    //////////////////////////////////////////////////////////////////////////
    // Step through this code line by line and verify the debug header
    // pins and button LEDs are going high/low
    //////////////////////////////////////////////////////////////////////////

    //Debug header
    debug.set(0, true);
    debug.set(0, false);
    debug.set(1, true);
    debug.set(1, false);
    debug.set(2, true);
    debug.set(2, false);
    debug.set(3, true);
    debug.set(3, false);

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


  void Process(int32_t *src, int32_t *dst, int size) {
      uint32_t 	i_sample;
      int32_t		in_L, in_R;
      int32_t 	sum_L=0, sum_R=0;

      if (do_audio_passthrough_test) {

        for ( i_sample = 0; i_sample < size; i_sample++) {
          in_L = *src++;
          in_R = *src++;

          sum_L += ((int32_t)(in_L<<8))/256;
          sum_R += ((int32_t)(in_R<<8))/256;

          *dst++ = in_L;
          *dst++ = in_R;
        }

        average_L = ((sum_L/size) << 8);
        average_R = ((sum_R/size) << 8);

      } else { //triangle wave test
        for ( i_sample = 0; i_sample < size; i_sample++) {
          if (tri_L_dir==1)	tri_L+=0x1000;
          else tri_L-=0x2000;

          if (tri_L >= MAX_CODEC_DAC_VAL) { tri_L_dir = 1 - tri_L_dir; tri_L = MAX_CODEC_DAC_VAL; }
          if (tri_L <= MIN_CODEC_DAC_VAL) { tri_L_dir = 1 - tri_L_dir; tri_L = MIN_CODEC_DAC_VAL; }

          if (tri_R_dir==1)	tri_R+=0x100000;
          else tri_R-=0x200000;

          if (tri_R >= MAX_CODEC_DAC_VAL) { tri_R_dir = 1 - tri_R_dir; tri_R = MAX_CODEC_DAC_VAL; }
          if (tri_R <= MIN_CODEC_DAC_VAL) { tri_R_dir = 1 - tri_R_dir; tri_R = MIN_CODEC_DAC_VAL; }

          *dst++ = tri_L;
          *dst++ = tri_R;
        }

      }
    }
} _;
