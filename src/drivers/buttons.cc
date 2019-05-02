#include "buttons.hh"

#include <cstdint>
#include "util.hh"
#include "hal.hh"

Buttons::Learn::Learn() {
  __HAL_RCC_GPIOC_CLK_ENABLE();
  GPIO_InitTypeDef gpio = {0};
  gpio.Pin = GPIO_PIN_9;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &gpio);
  for (int i=0; i<16; i++) Debounce();
}

bool Buttons::Learn::get() { return ReadPin(GPIOC, GPIO_PIN_9); };

Buttons::Freeze::Freeze() {
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitTypeDef gpio = {0};
  gpio.Pin = GPIO_PIN_11;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &gpio);
  for (int i=0; i<16; i++) Debounce();
}

bool Buttons::Freeze::get() { return ReadPin(GPIOA, GPIO_PIN_11); };
