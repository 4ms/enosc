#include "bl_ui.hh"

extern "C" {
#include "leds.h"
#include "buttons.h"
#include "bl_utils.h"
}
#include "bootloader.hh"

extern volatile uint32_t systmr;

// bool bootloader_entry_buttons_pushed(void)
// {
//  buttons.learn_.Debounce();
//  buttons.freeze_.Debounce();
//  return (buttons.learn_.pushed() && buttons.freeze_.pushed());
// }


bool button_pushed(enum Button button)
{
    if (button == BUTTON_LEARN) {
        return !PIN_READ(LEARN_BUT_GPIO_Port, LEARN_BUT_Pin);
    }
    else if (button == BUTTON_FREEZE) {
        return !PIN_READ(FREEZE_BUT_GPIO_Port, FREEZE_BUT_Pin);
    }
    else return false;
}

void animate_until_button_pushed(enum Animations animation_type, enum Button button)
{
    animate(ANI_RESET);

    while (!button_pushed(button))
    {   
        delay(1);
        animate(animation_type);
    }
    while (button_pushed(button)) {delay(1);}
}

void animate(enum Animations animation_type)
{
    uint32_t cur_tm = systmr;
    static uint32_t last_tm = 0;
    static uint8_t ctr = 0;
    uint32_t step_time = 500 * TICKS_PER_MS; //default

    switch (animation_type) {

        case ANI_RESET:
            SET_LEARN_OFF();
            SET_FREEZE_OFF();

            last_tm = cur_tm;
            ctr = 0;
            break;

        case ANI_SUCCESS:
            if (ctr==0) {
                SET_FREEZE_GREEN();
                SET_LEARN_YELLOW();
            }
            else if (ctr==1) {
                SET_FREEZE_YELLOW();
                SET_LEARN_GREEN();
            }
            // else if (ctr==2) {
            //     leds.freeze_.set(Colors::green);
            //     leds.learn_.set(Colors::green);
            // }
            // else if (ctr==3) {
            //     leds.freeze_.set(Colors::cyan);
            //     leds.learn_.set(Colors::cyan);
            // }
            // else if (ctr==4) {
            //     leds.freeze_.set(Colors::blue);
            //     leds.learn_.set(Colors::blue);
            // }
            // else if (ctr==5) {
            //     leds.freeze_.set(Colors::magenta);
            //     leds.learn_.set(Colors::magenta);
            // }
            // else if (ctr==6) {
            //     leds.freeze_.set(Colors::white);
            //     leds.learn_.set(Colors::white);
            // }
            else 
                ctr = 0;
            break;

        case ANI_WAITING:
            //Flash button green/off when waiting
            if (ctr==0)
                SET_LEARN_OFF();
            else if (ctr==1)
                SET_LEARN_GREEN();
            else
                ctr=0;
            break;

        case ANI_RECEIVING:
            step_time = 200*TICKS_PER_MS;
            if (ctr<3) {
                SET_FREEZE_BLUE();
                SET_LEARN_BLUE();
            } else if (ctr==3) {
                SET_FREEZE_BLUE();
                SET_LEARN_WHITE();
            } else if (ctr==4) {
                SET_FREEZE_WHITE();
                SET_LEARN_BLUE();
            } else
                ctr=0;
            break;

        case ANI_WRITING:
            step_time = 100*TICKS_PER_MS;
            if (ctr==0) {
                SET_FREEZE_YELLOW();
                SET_LEARN_OFF();
            } else if (ctr==1) {
                SET_FREEZE_OFF();
                SET_LEARN_YELLOW();
            } else
                ctr=0;
            break;

        case ANI_FAIL_ERR:
            step_time = 100*TICKS_PER_MS;
            if (ctr==0) {
                SET_LEARN_OFF();
                SET_FREEZE_RED();
            } else if (ctr==1) {
                SET_LEARN_RED();
                SET_FREEZE_RED();
            } else 
                ctr=0;
            break;

        default:
            break;
    }

    if ((cur_tm - last_tm) > step_time) {
        ctr++;
        last_tm = cur_tm;
    }
}
