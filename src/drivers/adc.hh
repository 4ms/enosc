#pragma once

#include "hal.hh"
#include "dsp.hh"

enum AdcInput {
  WARP_POT, DETUNE_POT, MOD_POT,
  ROOT_POT, GRID_POT, PITCH_POT,
  SPREAD_POT, TILT_POT, TWIST_POT,
  SPREAD_CV, WARP_CV, TWIST_CV,
  TILT_CV, GRID_CV, MOD_CV,
};

class Adc : Nocopy {
  enum RawAdcChannel {
    R_WARP_POT, R_DETUNE_POT, R_MOD_POT, R_ROOT_POT, R_GRID_POT,
    R_PITCH_POT, R_SPREAD_POT, R_TILT_POT, R_TWIST_POT,
    R_SPREAD_CV_1, R_WARP_CV, R_SPREAD_CV_2, R_TWIST_CV, R_TILT_CV, R_GRID_CV, R_MOD_CV,
    NUM_ADCS
  };
  static u0_16 values[NUM_ADCS];
  static ADC_HandleTypeDef hadc1;
  static ADC_HandleTypeDef hadc3;
  static DMA_HandleTypeDef hdma_adc1;
  static DMA_HandleTypeDef hdma_adc3;
  void ADC1_Init();
  void ADC3_Init();
public:
  Adc();
  void Start();
  void Wait();

  u0_16 get(AdcInput i) {
    switch(i) {
    case WARP_POT: return values[R_WARP_POT];
    case DETUNE_POT: return values[R_DETUNE_POT];
    case MOD_POT: return values[R_MOD_POT];
    case ROOT_POT: return values[R_ROOT_POT];
    case GRID_POT: return values[R_GRID_POT];
    case PITCH_POT: return values[R_PITCH_POT];
    case SPREAD_POT: return values[R_SPREAD_POT];
    case TILT_POT: return values[R_TILT_POT];
    case TWIST_POT: return values[R_TWIST_POT];
    case SPREAD_CV: return
          u0_16::wrap((u16_16(values[R_SPREAD_CV_1]) +
                       u16_16(values[R_SPREAD_CV_2])).div2<1>());
    case WARP_CV: return values[R_WARP_CV];
    case TWIST_CV: return values[R_TWIST_CV];
    case TILT_CV: return values[R_TILT_CV];
    case GRID_CV: return values[R_GRID_CV];
    case MOD_CV: return values[R_MOD_CV];
    default: return 0._u0_16;
    }
  }
};
