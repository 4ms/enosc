#include "periodic_func.h"
#include "stm32f7xx_ll_bus.h"
#include "stm32f7xx_ll_tim.h"

#define TIM_IT_IS_SET(x,y) 		(((x)->SR & (y)) == (y))
#define TIM_IT_IS_SOURCE(x,y) 	((((x)->DIER & (y)) == (y)) ? SET : RESET)
#define TIM_IT_CLEAR(x,y) 		(x)->SR &= ~(y)


void (*tim_callback)();

static uint8_t is_running;

void init_periodic_function(uint32_t period, uint32_t prescale, void cb())
{
	LL_TIM_InitTypeDef tim;

	is_running = 0;
	tim_callback = cb;

	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM4);

	tim.Prescaler = prescale;
	tim.CounterMode = LL_TIM_COUNTERMODE_UP;
	tim.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	tim.RepetitionCounter = 0;
	tim.Autoreload = period;
	LL_TIM_Init(TIM4, &tim);

  // 	LL_TIM_SetPrescaler(TIM4, prescale);
 	// LL_TIM_SetAutoReload(TIM4, period);

  	LL_TIM_EnableIT_UPDATE(TIM4);
  	uint32_t pri = NVIC_EncodePriority(5, 1, 1);
	NVIC_SetPriority(TIM4_IRQn, pri);
	NVIC_EnableIRQ(TIM4_IRQn);

  	LL_TIM_EnableCounter(TIM4);
  	LL_TIM_GenerateEvent_UPDATE(TIM4);
}

void TIM4_IRQHandler(void)
{
	// if (TIM_IT_IS_SET(TIM4, TIM_IT_UPDATE)){
	// 	if (TIM_IT_IS_SOURCE(TIM4, TIM_IT_UPDATE))
	// 	{


	if (LL_TIM_IsActiveFlag_UPDATE(TIM4)) {
		if ((tim_callback) && is_running)
			tim_callback();
	}
	LL_TIM_ClearFlag_UPDATE(TIM4);

		// Clear TIM update interrupt
		// TIM_IT_CLEAR(TIM4, TIM_IT_UPDATE);
	// }
}


void pause_periodic_func()
{
	is_running = 0;
}
void start_periodic_func()
{
	is_running = 1;
}

