#pragma once

struct Debug : Nocopy {
  Debug() {
    __HAL_RCC_GPIOD_CLK_ENABLE();

  GPIO_InitTypeDef gpio;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;

    gpio.Pin = GPIO_PIN_5;
    HAL_GPIO_Init(GPIOD, &gpio);

    gpio.Pin = GPIO_PIN_6;
    HAL_GPIO_Init(GPIOD, &gpio);

    gpio.Pin = GPIO_PIN_7;
    HAL_GPIO_Init(GPIOD, &gpio);

    gpio.Pin = GPIO_PIN_4;
    HAL_GPIO_Init(GPIOD, &gpio);
  }

  void set(int pin, bool value) {
    GPIO_PinState v = (GPIO_PinState)value;
    switch(pin) {
    case 0: HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, v); break;
    case 1: HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, v); break;
    case 2: HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, v); break;
    case 3: HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, v); break;
    }
  }
} debug;
