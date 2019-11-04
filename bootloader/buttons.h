#pragma once
#include "gpio_pins.h"

enum Button {
	BUTTON_LEARN,
	BUTTON_FREEZE
};

void init_buttons(void);

uint32_t learn_pressed(void);
uint32_t freeze_pressed(void);
void wait_for_freeze_released(void);
void wait_for_freeze_pressed(void);
void wait_for_learn_released(void);
void wait_for_learn_pressed(void);

