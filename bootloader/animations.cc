#include "animations.hh"
#include "leds.hh"
#include "bootloader.hh"

extern Leds leds;

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
			if (ctr==1) {
				leds.freeze_.set(Colors::yellow);
				leds.learn_.set(Colors::yellow);
			}
			if (ctr==2) {
				leds.freeze_.set(Colors::green);
				leds.learn_.set(Colors::green);
			}
			if (ctr==3) {
				leds.freeze_.set(Colors::cyan);
				leds.learn_.set(Colors::cyan);
			}
			if (ctr==4) {
				leds.freeze_.set(Colors::blue);
				leds.learn_.set(Colors::blue);
			}
			if (ctr==5) {
				leds.freeze_.set(Colors::magenta);
				leds.learn_.set(Colors::magenta);
			}
			if (ctr==6) {
				leds.freeze_.set(Colors::white);
				leds.learn_.set(Colors::white);
			}
			if (ctr==7)
				ctr = 0;
			break;

		case ANI_WAITING:
			//Flash button green/off when waiting
			step_time = 500*TICKS_PER_MS;

			if (ctr==0)
				leds.learn_.set(Colors::black);
			if (ctr==1)
				leds.learn_.set(Colors::green);
			if (ctr==2)
				ctr=0;
			break;

		case ANI_RECEIVING:
			step_time = 100*TICKS_PER_MS;

			if (ctr==0)
				leds.freeze_.set(Colors::white);
			if (ctr==1)
				leds.freeze_.set(Colors::blue);
			if (ctr==2)
				ctr=0;
			break;

		case ANI_FAIL_ERR:
			step_time = 100*TICKS_PER_MS;
			if (ctr==0) 
				leds.learn_.set(Colors::black);
			if (ctr==1) 
				leds.learn_.set(Colors::red);
			if (ctr==2)
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