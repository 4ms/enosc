#pragma once

#include <stm32f7xx.h>

void saw_out(int32_t *dst, uint32_t frames);
void set_saw_freqs(uint32_t new_freq_rise1, uint32_t new_freq_fall1, uint32_t new_freq_rise2, uint32_t new_freq_fall2);
void set_saw_ranges(uint32_t new_range_min1, uint32_t new_range_max1, uint32_t new_range_min2, uint32_t new_range_max2);
