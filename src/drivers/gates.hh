#pragma once

struct Gates : Nocopy {

  struct Learn {
    Learn() {
      __HAL_RCC_GPIOE_CLK_ENABLE();
      GPIO_InitTypeDef gpio = {0};
      gpio.Pin = GPIO_PIN_7;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLDOWN;
      HAL_GPIO_Init(GPIOE, &gpio);
    }
    inline bool get() { return HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_7); };
  } learn_;

  struct Freeze {
    Freeze() {
      __HAL_RCC_GPIOB_CLK_ENABLE();
      GPIO_InitTypeDef gpio = {0};
      gpio.Pin = GPIO_PIN_2;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLDOWN;
      HAL_GPIO_Init(GPIOB, &gpio);
    }
    inline bool get() { return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2); };
  } freeze_;
};
