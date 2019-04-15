#pragma once

extern "C" {
#include "stm32f7xx.h"
}

#define hal_assert(E) {                         \
    HAL_StatusTypeDef s = (E);                  \
    assert_param(s == HAL_OK);                  \
  }                                             \

inline void WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
  if(PinState != GPIO_PIN_RESET) {
    GPIOx->BSRR = GPIO_Pin;
  } else {
    GPIOx->BSRR = (uint32_t)GPIO_Pin << 16;
  }
}

inline GPIO_PinState ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
  return (GPIO_PinState)((GPIOx->IDR & GPIO_Pin) != 0);
}
