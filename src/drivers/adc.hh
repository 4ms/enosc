#pragma once

#include "hal.hh"
#include "dsp.hh"

enum AdcInput {
  POT_WARP, POT_DETUNE, POT_MOD,
  POT_ROOT, POT_GRID, POT_PITCH,
  POT_SPREAD, POT_TILT, POT_TWIST,
  CV_SPREAD, CV_WARP, CV_TWIST,
  CV_TILT, CV_GRID, CV_MOD,
  ADC_INPUT_MAX
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
    case POT_WARP: return values[R_WARP_POT];
    case POT_DETUNE: return values[R_DETUNE_POT];
    case POT_MOD: return values[R_MOD_POT];
    case POT_ROOT: return values[R_ROOT_POT];
    case POT_GRID: return values[R_GRID_POT];
    case POT_PITCH: return values[R_PITCH_POT];
    case POT_SPREAD: return values[R_SPREAD_POT];
    case POT_TILT: return values[R_TILT_POT];
    case POT_TWIST: return values[R_TWIST_POT];
    case CV_SPREAD: return
          u0_16::wrap((u16_16(values[R_SPREAD_CV_1]) +
                       u16_16(values[R_SPREAD_CV_2])).div2<1>());
    case CV_WARP: return values[R_WARP_CV];
    case CV_TWIST: return values[R_TWIST_CV];
    case CV_TILT: return values[R_TILT_CV];
    case CV_GRID: return values[R_GRID_CV];
    case CV_MOD: return values[R_MOD_CV];
    default: return 0._u0_16;
    }
  }
};
