#pragma once

#include "buttons.hh"

enum Animations {
	ANI_WAITING, 
	ANI_WRITING,
	ANI_RECEIVING,
	ANI_DONE,
	ANI_SUCCESS,
	ANI_FAIL_ERR,
	ANI_FAIL_SYNC,
	ANI_FAIL_CRC,
	ANI_RESET
};


void animate(enum Animations animation_type);
void animate_until_button_pushed(enum Animations animation_type, enum Button button);
