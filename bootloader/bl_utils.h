#pragma once

#include <stm32f7xx.h>

void delay(uint32_t ticks);
void SystemClock_Config(void);
void SetVectorTable(uint32_t reset_address);
void JumpTo(uint32_t address);
void *memcpy(void *dest, const void *src, unsigned int n);

void init_debug(void);

void write_flash_page(const uint8_t* data, uint32_t dst_addr, uint32_t bytes_to_write);
void copy_flash_page(uint32_t src_addr, uint32_t dst_addr, uint32_t bytes_to_copy);
void reset_buses(void);
void reset_RCC(void);