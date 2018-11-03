
#pragma once

#include "hal.hh"
#include "dsp.hh"

struct Adc {
  Adc();

  enum AdcChannel {
    WARP_POT,
    DETUNE_POT,
    MOD_POT,
    ROOT_POT,
    GRID_POT,
    PITCH_POT,
    SPREAD_POT,
    TILT_POT,
    TWIST_POT,
    SPREAD_CV_1,
    WARP_CV,
    SPREAD_CV_2,
    TWIST_CV,
    TILT_CV,
    GRID_CV,
    MOD_CV,
    NUM_ADCS
  };
  
  u0_16 get_adc(AdcChannel channel);

private:
  u0_16 adc_raw[NUM_ADCS];
  void ADC1_Init(u0_16 *adc_buffer, uint32_t num_channels);
  void ADC3_Init(u0_16 *adc_buffer, uint32_t num_channels);
};
