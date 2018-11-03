
#pragma once

#include "hal.hh"

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
  
  uint16_t get_adc(AdcChannel channel);

private:
  uint16_t adc_raw[NUM_ADCS];
  void ADC1_Init(uint16_t *adc_buffer, uint32_t num_channels);
  void ADC3_Init(uint16_t *adc_buffer, uint32_t num_channels);
};
