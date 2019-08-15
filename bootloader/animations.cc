#include "animations.hh"
#include "leds.hh"
#include "bootloader.hh"

extern Leds leds;
extern Buttons buttons;

void animate_until_button_pushed(enum Animations animation_type, enum Button button)
{
	animate(ANI_RESET);

	if (button == BUTTON_LEARN) {
		while (!buttons.learn_.pushed())
		{
			buttons.learn_.Debounce();
			animate(animation_type);
		}
		while (buttons.learn_.pushed())
			buttons.learn_.Debounce();
	}
	else if (button == BUTTON_FREEZE) {
		while (!buttons.freeze_.pushed())
		{
			buttons.freeze_.Debounce();
			animate(animation_type);
		}
		while (buttons.freeze_.pushed())
			buttons.freeze_.Debounce();
	}
}

void animate(enum Animations animation_type)
{
	uint32_t cur_tm = HAL_GetTick();
	static uint32_t last_tm = 0;
	static uint8_t ctr = 0;
	uint32_t step_time = 500 * TICKS_PER_MS; //default

	switch (animation_type) {

		case ANI_RESET:
			last_tm = cur_tm;
			ctr = 0;
			break;

		case ANI_SUCCESS:
			step_time = 500*TICKS_PER_MS;

			if (ctr==0) {
				leds.freeze_.set(Colors::red);
				leds.learn_.set(Colors::red);
			}
			else if (ctr==1) {
				leds.freeze_.set(Colors::yellow);
				leds.learn_.set(Colors::yellow);
			}
			else if (ctr==2) {
				leds.freeze_.set(Colors::green);
				leds.learn_.set(Colors::green);
			}
			else if (ctr==3) {
				leds.freeze_.set(Colors::cyan);
				leds.learn_.set(Colors::cyan);
			}
			else if (ctr==4) {
				leds.freeze_.set(Colors::blue);
				leds.learn_.set(Colors::blue);
			}
			else if (ctr==5) {
				leds.freeze_.set(Colors::magenta);
				leds.learn_.set(Colors::magenta);
			}
			else if (ctr==6) {
				leds.freeze_.set(Colors::white);
				leds.learn_.set(Colors::white);
			}
			else 
				ctr = 0;
			break;

		case ANI_WAITING:
			//Flash button green/off when waiting
			step_time = 500*TICKS_PER_MS;

			if (ctr==0)
				leds.learn_.set(Colors::black);
			else if (ctr==1)
				leds.learn_.set(Colors::green);
			else
				ctr=0;
			break;

		case ANI_RECEIVING:
			step_time = 200*TICKS_PER_MS;
			if (ctr<3) {
				leds.freeze_.set(Colors::blue);
				leds.learn_.set(Colors::blue);
			} else if (ctr==3) {
				leds.freeze_.set(Colors::white);
				leds.learn_.set(Colors::blue);
			} else if (ctr==4) {
				leds.freeze_.set(Colors::blue);
				leds.learn_.set(Colors::white);
			} else
				ctr=0;
			break;

		case ANI_WRITING:
			step_time = 200*TICKS_PER_MS;
			if (ctr==0) {
				leds.freeze_.set(Colors::yellow);
				leds.learn_.set(Colors::black);
			} else if (ctr==1) {
				leds.freeze_.set(Colors::black);
				leds.learn_.set(Colors::yellow);
			} else
				ctr=0;
			break;

		case ANI_FAIL_ERR:
			step_time = 100*TICKS_PER_MS;
			if (ctr==0) 
				leds.learn_.set(Colors::black);
			else if (ctr==1) 
				leds.learn_.set(Colors::red);
			else
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