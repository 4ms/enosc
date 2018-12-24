
#pragma once

#include "hal.hh"
#include "dsp.hh"

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
  void Start();

  struct Channel {
    AdcChannel ch;
    Channel(AdcChannel c) : ch(c) {}
    u0_16 get() { return value[(int)ch]; }
  };

  // Potentiometers
  Channel warp_pot {WARP_POT};
  Channel detune_pot {DETUNE_POT};
  Channel mod_pot {MOD_POT};
  Channel root_pot {ROOT_POT};
  Channel grid_pot {GRID_POT};
  Channel pitch_pot {PITCH_POT};
  Channel spread_pot {SPREAD_POT};
  Channel tilt_pot {TILT_POT};
  Channel twist_pot {TWIST_POT};

  // CV inputs
  struct SpreadCV : Channel {
    SpreadCV() : Channel(NUM_ADCS) {}
    u0_16 get() { return
        u0_16::narrow(u0_32(value[SPREAD_CV_1]) +
                      u0_32(value[SPREAD_CV_2]));
    }
  } spread_cv;
  Channel warp_cv {WARP_CV};
  Channel twist_cv {TWIST_CV};
  Channel tilt_cv {TILT_CV};
  Channel grid_cv {GRID_CV};
  Channel mod_cv {MOD_CV};
};
