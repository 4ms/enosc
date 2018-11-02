#pragma once

#include "stm32f7xx.h"

#define hal_assert(E)                           \
  if ((E) != HAL_OK ) {                         \
    assert_failed(__FILE__, __LINE__);          \
  }
