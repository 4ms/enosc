#pragma once

#include "hal.hh"
#include "dsp.hh"

// ADC1
enum Adc1Channels {
	WARP_POT_ADC,		//  0
	DETUNE_POT_ADC,		//  1
	MOD_POT_ADC,		//  2
	ROOT_POT_ADC, 		//  3
	GRID_POT_ADC,		//  4
	PITCH_POT_ADC,		//  5
	SPREAD_POT_ADC,		//  6
	TILT_POT_ADC,		//  7
	TWIST_POT_ADC,		//  8

	NUM_ADC1
};

// ADC3
enum Adc3Channels{
	SPREAD_CV_1_ADC,	//  0
	WARP_CV_ADC,		//  1
	SPREAD_CV_2_ADC,	//  2
	TWIST_CV_ADC, 		//  3
	TILT_CV_ADC,		//  4
	GRID_CV_ADC,		//  5
	MOD_CV_ADC,			//  6
	
	NUM_ADC3
};

class Adc : Nocopy {
  enum AdcChannel {
    WARP_POT, DETUNE_POT, MOD_POT, ROOT_POT, GRID_POT,
    PITCH_POT, SPREAD_POT, TILT_POT, TWIST_POT,
    SPREAD_CV_1, WARP_CV, SPREAD_CV_2, TWIST_CV, TILT_CV, GRID_CV, MOD_CV,
    NUM_ADCS
  };
  static u0_16 value[NUM_ADCS];
  static ADC_HandleTypeDef hadc1;
  static ADC_HandleTypeDef hadc3;
  static DMA_HandleTypeDef hdma_adc1;
  static DMA_HandleTypeDef hdma_adc3;
  void ADC1_Init();
  void ADC3_Init();
public:
  Adc();
  void Start() {
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)value, NUM_ADC1);
    HAL_ADC_Start_DMA(&hadc3, (uint32_t*)(value + NUM_ADC1), NUM_ADC3);
  };
  void Wait();

  // Potentiometers
  u0_16 warp_pot() { return value[WARP_POT]; }
  u0_16 detune_pot() { return value[DETUNE_POT]; }
  u0_16 mod_pot() { return value[MOD_POT]; }
  u0_16 root_pot() { return value[ROOT_POT]; }
  u0_16 grid_pot() { return value[GRID_POT]; }
  u0_16 pitch_pot() { return value[PITCH_POT]; }
  u0_16 spread_pot() { return value[SPREAD_POT]; }
  u0_16 tilt_pot() { return value[TILT_POT]; }
  u0_16 twist_pot() { return value[TWIST_POT]; }
  u0_16 spread_cv() { return
      u0_16::wrap((u16_16(value[SPREAD_CV_1]) +
                     u16_16(value[SPREAD_CV_2])).div2<1>());
    }

  u0_16 warp_cv() { return value[WARP_CV]; }
  u0_16 twist_cv() { return value[TWIST_CV]; }
  u0_16 tilt_cv() { return value[TILT_CV]; }
  u0_16 grid_cv() { return value[GRID_CV]; }
  u0_16 mod_cv() { return value[MOD_CV]; }
};
