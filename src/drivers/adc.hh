#pragma once

#include "hal.hh"
#include "dsp.hh"

enum AdcInput {
  POT_WARP, POT_DETUNE, POT_MOD,
  POT_ROOT, POT_SCALE, POT_PITCH,
  POT_SPREAD, POT_BALANCE, POT_TWIST,
  CV_SPREAD, CV_WARP, CV_TWIST,
  CV_BALANCE, CV_SCALE, CV_MOD,
  ADC_INPUT_MAX
};

class Adc : Nocopy {
  enum RawAdcChannel {
    R_WARP_POT, R_DETUNE_POT, R_MOD_POT, R_ROOT_POT, R_SCALE_POT,
    R_PITCH_POT, R_SPREAD_POT, R_BALANCE_POT, R_TWIST_POT,
    R_SPREAD_CV, R_WARP_CV, R_MOD_CV_1, R_TWIST_CV, R_BALANCE_CV, R_SCALE_CV, R_MOD_CV_2,
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
    case POT_SCALE: return values[R_SCALE_POT];
    case POT_PITCH: return values[R_PITCH_POT];
    case POT_SPREAD: return values[R_SPREAD_POT];
    case POT_BALANCE: return values[R_BALANCE_POT];
    case POT_TWIST: return values[R_TWIST_POT];
    case CV_SPREAD: return values[R_SPREAD_CV];
    case CV_WARP: return values[R_WARP_CV];
    case CV_TWIST: return values[R_TWIST_CV];
    case CV_BALANCE: return values[R_BALANCE_CV];
    case CV_SCALE: return values[R_SCALE_CV];
    case CV_MOD: return 
          u0_16::wrap((u16_16(values[R_MOD_CV_1]) +
                       u16_16(values[R_MOD_CV_2])).div2<1>());

    default: return 0._u0_16;
    }
  }
};
