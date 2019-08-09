#include "buttons.hh"

#include <cstdint>
#include "util.hh"
#include "hal.hh"

#define LEARN_BUT_Pin GPIO_PIN_9
#define LEARN_BUT_GPIO_Port GPIOC 
#define LEARN_BUT_RCC_CLK_ON __HAL_RCC_GPIOC_CLK_ENABLE

#define FREEZE_BUT_Pin GPIO_PIN_11
#define FREEZE_BUT_GPIO_Port GPIOA
#define FREEZE_BUT_RCC_CLK_ON __HAL_RCC_GPIOA_CLK_ENABLE


Buttons::Learn::Learn() {
  LEARN_BUT_RCC_CLK_ON();
  GPIO_InitTypeDef gpio = {0};
  gpio.Pin = LEARN_BUT_Pin;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(LEARN_BUT_GPIO_Port, &gpio);
  for (int i=0; i<16; i++) Debounce();
}

bool Buttons::Learn::get() { return ReadPin(LEARN_BUT_GPIO_Port, LEARN_BUT_Pin); };

Buttons::Freeze::Freeze() {
  FREEZE_BUT_RCC_CLK_ON();
  GPIO_InitTypeDef gpio = {0};
  gpio.Pin = FREEZE_BUT_Pin;
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(FREEZE_BUT_GPIO_Port, &gpio);
  for (int i=0; i<16; i++) Debounce();
}

bool Buttons::Freeze::get() { return ReadPin(FREEZE_BUT_GPIO_Port, FREEZE_BUT_Pin); };
