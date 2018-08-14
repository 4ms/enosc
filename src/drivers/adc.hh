#include "stm32f4xx.h"

class Adc {
    ADC_HandleTypeDef hadc_ = {0};
public:
  Adc() {
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_1;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Speed = GPIO_SPEED_FAST;
    gpio.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    __HAL_RCC_ADC1_CLK_ENABLE();

    hadc_.Instance = ADC1;
    hadc_.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc_.Init.Resolution = ADC_RESOLUTION_12B;
    hadc_.Init.DataAlign = ADC_DATAALIGN_LEFT;
    hadc_.Init.ScanConvMode = DISABLE;
    hadc_.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc_.Init.ContinuousConvMode = DISABLE;
    hadc_.Init.NbrOfConversion = 1;
    hadc_.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    HAL_ADC_Init(&hadc_);

    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel = ADC_CHANNEL_1;
    ch.Rank = 1;
    ch.SamplingTime = ADC_SAMPLETIME_144CYCLES;
    HAL_ADC_ConfigChannel(&hadc_, &ch);
  }

  uint16_t Read() {
    if (HAL_ADC_Start(&hadc_) != HAL_OK)
      while(1);
    while(HAL_ADC_PollForConversion(&hadc_, 1000) != HAL_OK);
    uint16_t val = HAL_ADC_GetValue(&hadc_);
    if (HAL_ADC_Stop(&hadc_) != HAL_OK)
      while(1);
    return val;
  }
};
