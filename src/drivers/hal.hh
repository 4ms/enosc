#pragma once

#include "stm32f7xx.h"

#define hal_assert(E) {                         \
    HAL_StatusTypeDef s = (E);                  \
    assert_param(s == HAL_OK);                  \
  }                                             \

