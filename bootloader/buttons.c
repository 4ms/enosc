#include "buttons.h"
#include "bl_utils.h"

void init_buttons(void)
{
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN);
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOCEN);

    LL_GPIO_SetPinMode(LEARN_BUT_GPIO_Port, LEARN_BUT_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(LEARN_BUT_GPIO_Port, LEARN_BUT_Pin, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinMode(FREEZE_BUT_GPIO_Port, FREEZE_BUT_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(FREEZE_BUT_GPIO_Port, FREEZE_BUT_Pin, LL_GPIO_PULL_UP);
}

// static inline uint32_t learn_pressed(void) { return !PIN_READ(LEARN_BUT_GPIO_Port, LEARN_BUT_Pin); }
// static inline uint32_t freeze_pressed(void) { return !PIN_READ(FREEZE_BUT_GPIO_Port, FREEZE_BUT_Pin); }
uint32_t learn_pressed(void) { return !PIN_READ(LEARN_BUT_GPIO_Port, LEARN_BUT_Pin); }
uint32_t freeze_pressed(void) { return !PIN_READ(FREEZE_BUT_GPIO_Port, FREEZE_BUT_Pin); }

void wait_for_freeze_released(void)  { while (freeze_pressed()) {;}; delay(1000); }
void wait_for_freeze_pressed(void)  { while (!freeze_pressed()) {;}; delay(1000); }
void wait_for_learn_released(void)  { while (learn_pressed()) {;}; delay(1000); }
void wait_for_learn_pressed(void)  { while (!learn_pressed()) {;}; delay(1000); }
