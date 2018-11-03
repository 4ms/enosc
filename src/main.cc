extern "C" {
#include "hal.hh"
#include "globals.h"
#include "codec.h"
#include "audio_stream.h"
}

#include "system.hh"
#include "buttons.hh"
#include "gates.hh"
#include "switches.hh"
#include "debug.hh"
#include "leds.hh"
#include "adc.hh"

bool do_audio_passthrough_test = false;


struct Main {
  System sys_;
  Buttons buttons_;
  Gates gates_;
  Switches switches_;
  Leds leds_;
  Adc adc_;

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

    // Setup PLL clock for codec
   	init_SAI_clock(SAMPLERATE);

   	//De-init the codec to force it to reset
    codec_deinit();
    HAL_Delay(10);

    //Start Codec I2C
    codec_GPIO_init();
    codec_I2C_init();

    HAL_Delay(100);
    if (codec_register_setup(SAMPLERATE))
      assert_failed(__FILE__, __LINE__);

    //Start Codec SAI
    codec_SAI_init(SAMPLERATE);
    init_audio_DMA();

    //Start audio processing
    start_audio();

    // process
    while(1) {
      Process();
    }
  }


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
} _;
