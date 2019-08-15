#include "periodic_func.hh"
#include "bootloader.hh"

#define TIM_IT_IS_SET(x,y) 		(((x)->SR & (y)) == (y))
#define TIM_IT_IS_SOURCE(x,y) 	((((x)->DIER & (y)) == (y)) ? SET : RESET)
#define TIM_IT_CLEAR(x,y) 		(x)->SR &= ~(y)


void (*tim_callback)();

static uint8_t is_running;

void init_periodic_function(uint32_t period, uint32_t prescale, void cb())
{
	TIM_HandleTypeDef tim;

	__HAL_RCC_TIM4_CLK_ENABLE();
	HAL_NVIC_SetPriority(TIM4_IRQn, 2, 1);

	//12kHz
	tim.Init.Period 			= period;
	tim.Init.Prescaler 			= prescale;
	tim.Init.ClockDivision 		= 0;
	tim.Init.CounterMode 		= TIM_COUNTERMODE_UP;
	tim.Init.RepetitionCounter 	= 0;
	tim.Init.AutoReloadPreload 	= TIM_AUTORELOAD_PRELOAD_DISABLE;
	tim.Instance 				= TIM4;

	HAL_NVIC_EnableIRQ(TIM4_IRQn);
	HAL_TIM_Base_Init(&tim);
	HAL_TIM_Base_Start_IT(&tim);

	is_running = 0;
	tim_callback = cb;
}

extern "C" { void TIM4_IRQHandler(void)
{
	if (TIM_IT_IS_SET(TIM4, TIM_IT_UPDATE)){
		if (TIM_IT_IS_SOURCE(TIM4, TIM_IT_UPDATE))
		{
			if ((tim_callback != NULL) && is_running)
				tim_callback();
		}
		// Clear TIM update interrupt
		TIM_IT_CLEAR(TIM4, TIM_IT_UPDATE);
	}
}
}

void pause_periodic_func()
{
	is_running = 0;
}
void start_periodic_func()
{
	is_running = 1;
}

