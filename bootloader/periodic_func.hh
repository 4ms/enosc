#pragma once

#include <stm32f7xx.h>

void init_periodic_function(uint32_t period, uint32_t prescale, void cb());

void start_periodic_func();
void pause_periodic_func();

