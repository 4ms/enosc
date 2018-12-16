#pragma once

#include "stm32f7xx.h"

#define hal_assert(E)                           \
  assert_param((E) == HAL_OK);                  \


