#include "leds.h"

void init_leds(void) {

    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOCEN);

    LL_GPIO_SetPinMode(LEARN_RED_GPIO_Port, LEARN_RED_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(LEARN_RED_GPIO_Port, LEARN_RED_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinMode(LEARN_GREEN_GPIO_Port, LEARN_GREEN_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(LEARN_GREEN_GPIO_Port, LEARN_GREEN_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinMode(LEARN_BLUE_GPIO_Port, LEARN_BLUE_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(LEARN_BLUE_GPIO_Port, LEARN_BLUE_Pin, LL_GPIO_OUTPUT_PUSHPULL);

    LL_GPIO_SetPinMode(FREEZE_RED_GPIO_Port, FREEZE_RED_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(FREEZE_RED_GPIO_Port, FREEZE_RED_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinMode(FREEZE_GREEN_GPIO_Port, FREEZE_GREEN_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(FREEZE_GREEN_GPIO_Port, FREEZE_GREEN_Pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinMode(FREEZE_BLUE_GPIO_Port, FREEZE_BLUE_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinOutputType(FREEZE_BLUE_GPIO_Port, FREEZE_BLUE_Pin, LL_GPIO_OUTPUT_PUSHPULL);
}
