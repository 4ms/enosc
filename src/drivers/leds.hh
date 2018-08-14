#include "stm32f4xx.h"

enum LED {
  LED_1,
  LED_2,
  LED_3,
  LED_4,
};

struct Leds
{
  Leds() {
    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitTypeDef init;
    init.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    init.Mode = GPIO_MODE_OUTPUT_PP;
    init.Speed = GPIO_SPEED_LOW;
    init.Pull = GPIO_PULLUP;

    HAL_GPIO_Init(GPIOD, &init);

    set(LED_1, 0);
    set(LED_2, 0);
    set(LED_3, 0);
    set(LED_4, 0);
  }

  void set(LED led, bool status) {
    GPIO_PinState s = static_cast<GPIO_PinState>(status);
    if (led == LED_1) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, s);
    if (led == LED_2) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, s);
    if (led == LED_3) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, s);
    if (led == LED_4) HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, s);
  }

  void toggle(LED led) {
    if (led == LED_1) HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_12);
    if (led == LED_2) HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_13);
    if (led == LED_3) HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
    if (led == LED_4) HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15);
  }

};
