#pragma once
#include "gpio_pins.h"

#define ADCSPIx SPI2
#define ADCSPI_IRQn SPI2_IRQn
#define ADCSPI_IRQHandler SPI2_IRQHandler

#define ADCSPI_GPIO GPIOB
#define ADCSPI_GPIO_RCC LL_AHB1_GRP1_PERIPH_GPIOB
#define ADCSPI_GPIO_AF LL_GPIO_AF_5
#define ADCSPI_SCK_pin LL_GPIO_PIN_13
#define ADCSPI_MISO_pin LL_GPIO_PIN_14
#define ADCSPI_CHSEL_pin LL_GPIO_PIN_15
#define ADCSPI_CS_pin LL_GPIO_PIN_12


enum max11666Commands {
  MAX11666_SWITCH_TO_CH1 = 0x00FF,
  MAX11666_CONTINUE_READING_CH1 = 0x0000,
  MAX11666_SWITCH_TO_CH2 = 0xFF00,
  MAX11666_CONTINUE_READING_CH2 = 0xFFFF,
};

enum max11666Errors {
	MAX11666_NO_ERR = 0,
	MAX11666_SPI_INIT_ERR,
	MAX11666_SPI_OVR_ERR
};

uint16_t get_adc_spi_value(void);
void init_adc_spi(void);
void set_adc_spi_channel(uint8_t chan);
