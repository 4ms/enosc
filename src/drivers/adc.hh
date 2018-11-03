
#pragma once

#include "hal.hh"

// ADC1
enum AdcChannel {
  WARP_POT,		//  0
  DETUNE_POT,		//  1
  MOD_POT,		//  2
  ROOT_POT, 		//  3
  GRID_POT,		//  4
  PITCH_POT,		//  5
  SPREAD_POT,		//  6
  TILT_POT,		//  7
  TWIST_POT,		//  8
  SPREAD_CV_1,	//  0
  WARP_CV,		//  1
  SPREAD_CV_2,	//  2
  TWIST_CV, 		//  3
  TILT_CV,		//  4
  GRID_CV,		//  5
  MOD_CV,			//  6
	
  NUM_ADCS
};

struct Adc {
  Adc();
  uint16_t get_adc(AdcChannel channel);

private:
  uint16_t adc_raw[NUM_ADCS];
  void ADC1_Init(uint16_t *adc_buffer, uint32_t num_channels);
  void ADC3_Init(uint16_t *adc_buffer, uint32_t num_channels);
};
