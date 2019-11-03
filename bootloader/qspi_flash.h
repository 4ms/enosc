#pragma once
#include <stm32f7xx.h>
#include "qspi_flash_ll.h"

void init_QSPI(void);
void init_QSPI_GPIO_1IO(void);
void init_QSPI_GPIO_4IO(void);
