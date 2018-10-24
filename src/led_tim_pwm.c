/*
 * led_tim_pwm.c - PWM output for the channel ENV OUT jacks
 *
 * Author: Dan Green (danngreen1@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * See http://creativecommons.org/licenses/MIT/ for more information.
 *
 * -----------------------------------------------------------------------------
 */

#include "led_tim_pwm.h"
#include "globals.h"
#include "gpio_pins.h"
#include "hal_handlers.h"
#include "adc_interface.h"

TIM_HandleTypeDef	timFREEZELED;
TIM_HandleTypeDef	timLEARNLED;

extern uint16_t	builtin_adc1_raw[ NUM_BUILTIN_ADC1 ];

void init_led_tim_pwm(void)
{
	GPIO_InitTypeDef 	gpio;
	TIM_OC_InitTypeDef	tim_oc;

	LED_PWM_RCC_ENABLE();

	//
	// Setup GPIO for timer output pins
	//
	LED_PWM_TIM_GPIO_RCC_ENABLE();

	gpio.Mode 	= GPIO_MODE_AF_PP;
	gpio.Pull 	= GPIO_PULLUP;
	gpio.Speed 	= GPIO_SPEED_FREQ_HIGH;

	gpio.Alternate 	= FREEZE_LED_PWM_TIM_AF;
	gpio.Pin 		= FREEZE_LED_PWM_pins;
	HAL_GPIO_Init(FREEZE_LED_PWM_GPIO, &gpio);

	gpio.Alternate 	= LEARN_LED_PWM_TIM_AF;
	gpio.Pin 		= LEARN_LED_PWM_pins;
	HAL_GPIO_Init(LEARN_LED_PWM_GPIO, &gpio);

	// Initialize the Timer peripherals (period determines resolution and frequency)

	timFREEZELED.Instance 				 	= FREEZE_LED_PWM_TIM;
	timFREEZELED.Init.Prescaler         	= 0;
	timFREEZELED.Init.Period            	= PWM_MAX; //216M / 1 / 256 = 840kHz;
	timFREEZELED.Init.ClockDivision     	= 0;
	timFREEZELED.Init.CounterMode       	= TIM_COUNTERMODE_UP;
	timFREEZELED.Init.RepetitionCounter 	= 0;
	timFREEZELED.Init.AutoReloadPreload 	= TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_PWM_Init(&timFREEZELED) != HAL_OK) _Error_Handler(__FILE__, __LINE__);


	timLEARNLED.Instance 					= LEARN_LED_PWM_TIM;
	timLEARNLED.Init.Prescaler         		= 0;
	timLEARNLED.Init.Period            		= PWM_MAX; //216M / 2 / 256 = 420kHz;
	timLEARNLED.Init.ClockDivision     		= 0;
	timLEARNLED.Init.CounterMode       		= TIM_COUNTERMODE_UP;
	timLEARNLED.Init.RepetitionCounter 		= 0;
	timLEARNLED.Init.AutoReloadPreload 		= TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_PWM_Init(&timLEARNLED) != HAL_OK) _Error_Handler(__FILE__, __LINE__);


	// Configure each TIMx peripheral's Output Compare units.
	// Each channel (CCRx) needs to be enabled for each TIMx that we're using

	//Common configuration for all channels
	tim_oc.OCMode       = TIM_OCMODE_PWM1;
	tim_oc.OCPolarity   = TIM_OCPOLARITY_HIGH;
	tim_oc.OCFastMode   = TIM_OCFAST_DISABLE;
	tim_oc.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
	tim_oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	tim_oc.OCIdleState  = TIM_OCIDLESTATE_RESET;
	tim_oc.Pulse 		= 0;

	if (HAL_TIM_PWM_ConfigChannel(&timFREEZELED, &tim_oc, 	FREEZE_LED_PWM_CHAN_RED) != HAL_OK)		_Error_Handler(__FILE__, __LINE__);
	if (HAL_TIM_PWM_ConfigChannel(&timFREEZELED, &tim_oc, 	FREEZE_LED_PWM_CHAN_GREEN) != HAL_OK)	_Error_Handler(__FILE__, __LINE__);
	if (HAL_TIM_PWM_ConfigChannel(&timFREEZELED, &tim_oc, 	FREEZE_LED_PWM_CHAN_BLUE) != HAL_OK)	_Error_Handler(__FILE__, __LINE__);

	if (HAL_TIM_PWM_ConfigChannel(&timLEARNLED, &tim_oc, 	LEARN_LED_PWM_CHAN_RED) != HAL_OK)		_Error_Handler(__FILE__, __LINE__);
	if (HAL_TIM_PWM_ConfigChannel(&timLEARNLED, &tim_oc, 	LEARN_LED_PWM_CHAN_GREEN) != HAL_OK)	_Error_Handler(__FILE__, __LINE__);
	if (HAL_TIM_PWM_ConfigChannel(&timLEARNLED, &tim_oc, 	LEARN_LED_PWM_CHAN_BLUE) != HAL_OK)		_Error_Handler(__FILE__, __LINE__);

	//
	// Start PWM signals generation
	//
 	if (HAL_TIM_PWM_Start(&timFREEZELED, 	FREEZE_LED_PWM_CHAN_RED) != HAL_OK)						_Error_Handler(__FILE__, __LINE__);
 	if (HAL_TIM_PWM_Start(&timFREEZELED, 	FREEZE_LED_PWM_CHAN_GREEN) != HAL_OK)					_Error_Handler(__FILE__, __LINE__);
 	if (HAL_TIM_PWM_Start(&timFREEZELED, 	FREEZE_LED_PWM_CHAN_BLUE) != HAL_OK)					_Error_Handler(__FILE__, __LINE__);
 
 	if (HAL_TIM_PWM_Start(&timLEARNLED, 	LEARN_LED_PWM_CHAN_RED) != HAL_OK)						_Error_Handler(__FILE__, __LINE__);
 	if (HAL_TIM_PWM_Start(&timLEARNLED, 	LEARN_LED_PWM_CHAN_GREEN) != HAL_OK)					_Error_Handler(__FILE__, __LINE__);
 	if (HAL_TIM_PWM_Start(&timLEARNLED, 	LEARN_LED_PWM_CHAN_BLUE) != HAL_OK)						_Error_Handler(__FILE__, __LINE__);

}

void update_led_tim_pwm(void)
{
	uint8_t ledpwm[6]={0};
	uint8_t i;

	for (i=0;i<6;i++) ledpwm[i] = builtin_adc1_raw[i]/(4096/PWM_MAX);

	FREEZE_LED_PWM_TIM->FREEZE_LED_PWM_CC_RED 	= ledpwm[0];
	FREEZE_LED_PWM_TIM->FREEZE_LED_PWM_CC_GREEN = ledpwm[1];
	FREEZE_LED_PWM_TIM->FREEZE_LED_PWM_CC_BLUE 	= ledpwm[2];

	LEARN_LED_PWM_TIM->LEARN_LED_PWM_CC_RED 	= ledpwm[3];
	LEARN_LED_PWM_TIM->LEARN_LED_PWM_CC_GREEN 	= ledpwm[4];
	LEARN_LED_PWM_TIM->LEARN_LED_PWM_CC_BLUE 	= ledpwm[5];
}
