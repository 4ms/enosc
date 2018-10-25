#pragma once

#define DEBUG1_OUT_Pin GPIO_PIN_5
#define DEBUG1_OUT_GPIO_Port GPIOD

#define DEBUG2_OUT_Pin GPIO_PIN_6
#define DEBUG2_OUT_GPIO_Port GPIOD

#define DEBUG3_OUT_Pin GPIO_PIN_7
#define DEBUG3_OUT_GPIO_Port GPIOD

#define DEBUG4_OUT_Pin GPIO_PIN_4
#define DEBUG4_OUT_GPIO_Port GPIOD

struct Debug {
  Debug() {
    GPIO_InitTypeDef gpio;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;

    gpio.Pin = DEBUG1_OUT_Pin;
    HAL_GPIO_Init(DEBUG1_OUT_GPIO_Port, &gpio);

    gpio.Pin = DEBUG2_OUT_Pin;
    HAL_GPIO_Init(DEBUG2_OUT_GPIO_Port, &gpio);

    gpio.Pin = DEBUG3_OUT_Pin;
    HAL_GPIO_Init(DEBUG3_OUT_GPIO_Port, &gpio);

    gpio.Pin = DEBUG4_OUT_Pin;
    HAL_GPIO_Init(DEBUG4_OUT_GPIO_Port, &gpio);
  }

  void set(int pin, bool value) {
    switch(pin) {
    case 0:
      HAL_GPIO_WritePin(DEBUG1_OUT_GPIO_Port, DEBUG1_OUT_Pin, (GPIO_PinState)value);
      break;
    case 1:
      HAL_GPIO_WritePin(DEBUG2_OUT_GPIO_Port, DEBUG2_OUT_Pin, (GPIO_PinState)value);
      break;
    case 2:
      HAL_GPIO_WritePin(DEBUG3_OUT_GPIO_Port, DEBUG3_OUT_Pin, (GPIO_PinState)value);
      break;
    case 3:
      HAL_GPIO_WritePin(DEBUG4_OUT_GPIO_Port, DEBUG4_OUT_Pin, (GPIO_PinState)value);
      break;
    }
  }
} debug_;
