/*
 * led_tim_pwm.h
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

#pragma once

#include <stm32f7xx.h>

#define PWM_MAX 		256	// Maximum PWM value

// PWM OUTs

#define FREEZE_LED_PWM_TIM			TIM1

#define FREEZE_LED_PWM_CC_RED 		CCR1
#define FREEZE_LED_PWM_CC_GREEN 	CCR2
#define FREEZE_LED_PWM_CC_BLUE 		CCR3

#define FREEZE_LED_PWM_CHAN_RED 	TIM_CHANNEL_1
#define FREEZE_LED_PWM_CHAN_GREEN 	TIM_CHANNEL_2
#define FREEZE_LED_PWM_CHAN_BLUE 	TIM_CHANNEL_3

#define FREEZE_LED_PWM_TIM_AF		GPIO_AF1_TIM1
#define FREEZE_LED_PWM_pins 		(FREEZE_RED_Pin | FREEZE_GREEN_Pin | FREEZE_BLUE_Pin)
#define FREEZE_LED_PWM_GPIO 		FREEZE_RED_GPIO_Port

#define LEARN_LED_PWM_TIM			TIM3

#define LEARN_LED_PWM_CC_RED 		CCR1
#define LEARN_LED_PWM_CC_GREEN 		CCR2
#define LEARN_LED_PWM_CC_BLUE 		CCR3

#define LEARN_LED_PWM_CHAN_RED 		TIM_CHANNEL_1
#define LEARN_LED_PWM_CHAN_GREEN 	TIM_CHANNEL_2
#define LEARN_LED_PWM_CHAN_BLUE 	TIM_CHANNEL_3

#define LEARN_LED_PWM_TIM_AF		GPIO_AF2_TIM3
#define LEARN_LED_PWM_pins 			(LEARN_RED_Pin | LEARN_GREEN_Pin | LEARN_BLUE_Pin)
#define LEARN_LED_PWM_GPIO 			LEARN_RED_GPIO_Port


#define LED_PWM_TIM_GPIO_RCC_ENABLE()		__HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOC_CLK_ENABLE();
#define LED_PWM_RCC_ENABLE()				__HAL_RCC_TIM1_CLK_ENABLE(); __HAL_RCC_TIM3_CLK_ENABLE();

void init_led_tim_pwm(void);
void update_led_tim_pwm(void);
