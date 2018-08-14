#include "stm32f4xx.h"

class Button {
  uint8_t state_;
public:
  Button() {
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef init;
    init.Pin = GPIO_PIN_0;
    init.Mode = GPIO_MODE_INPUT;
    init.Speed = GPIO_SPEED_LOW;
    init.Pull = GPIO_NOPULL;

    HAL_GPIO_Init(GPIOD, &init);
  }

  void Read() {
    state_ = (state_ << 1) | (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) != 0);
  }

  bool pressed() { return state_ == 0b11111111; }
  bool just_pressed() { return state_ == 0b01111111; }
  bool just_released() { return state_ == 0b10000000; }
};
