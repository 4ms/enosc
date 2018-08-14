#include "stm32f4xx.h"

class Rng {
  RNG_HandleTypeDef hrng_ = {.Instance = RNG};

public:
  Rng() {
    __HAL_RCC_RNG_CLK_ENABLE();
    HAL_RNG_Init(&hrng_);
  }

  uint32_t Bits() {
    uint32_t val;
    if (HAL_RNG_GenerateRandomNumber(&hrng_, &val) != HAL_OK)
      while(1);
    return val;
  }

  int16_t Int16() {
    return static_cast<int16_t>(Bits());
  }

  // returns float between 0 and 1
  float Float() {
    float f = static_cast<float>(Int16()) / INT16_MAX;
  }
};
