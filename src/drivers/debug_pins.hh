#include "stm32f4xx.h"

struct DebugPins
{
  static constexpr int kNumPins = 4;

  DebugPins() {
    __HAL_RCC_GPIOE_CLK_ENABLE();

    GPIO_InitTypeDef init;
    init.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5;
    init.Mode = GPIO_MODE_OUTPUT_PP;
    init.Speed = GPIO_SPEED_LOW;
    init.Pull = GPIO_PULLUP;

    HAL_GPIO_Init(GPIOE, &init);

    for (int i=1; i<kNumPins+1; i++)
      set(i, false);
  }

  void set(int pin, bool status) {
    GPIO_PinState s = static_cast<GPIO_PinState>(status);
    if (pin == 1) HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, s);
    if (pin == 2) HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, s);
    if (pin == 3) HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, s);
    if (pin == 4) HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, s);
  }

  void toggle(int pin) {
    if (pin == 1) HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);
    if (pin == 2) HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_2);
    if (pin == 3) HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_4);
    if (pin == 4) HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_5);
  }

  void on(int l) { set(l, true); }
  void off(int l) { set(l, false); }
};

DebugPins debug;
