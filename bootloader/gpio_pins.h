/*
 * gpio_pins.h - setup digital input and output pins
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
#include "stm32f7xx_ll_gpio.h"

#define PIN_ON(x,y)     x->BSRR = y
#define PIN_OFF(x,y)    x->BSRR = (uint32_t)y << 16
#define PIN_READ(x, y) (((x->IDR) & y) ? 1 : 0)
#define PORT_READ(x, y) ((x->IDR) & y)

//##############################################################################
//                              GPIOs and PINS 
//##############################################################################

// Gate jacks

#define FREEZE_JACK_Pin LL_GPIO_PIN_9
#define FREEZE_JACK_GPIO_Port GPIOB

#define LEARN_JACK_Pin LL_GPIO_PIN_8
#define LEARN_JACK_GPIO_Port GPIOB


//Switches/Buttons

#define CROSSFMSW_TOP_Pin LL_GPIO_PIN_14
#define CROSSFMSW_TOP_GPIO_Port GPIOE
#define CROSSFMSW_BOT_Pin LL_GPIO_PIN_15
#define CROSSFMSW_BOT_GPIO_Port GPIOE

#define SCALESW_TOP_Pin LL_GPIO_PIN_10
#define SCALESW_TOP_GPIO_Port GPIOB
#define SCALESW_BOT_Pin LL_GPIO_PIN_11
#define SCALESW_BOT_GPIO_Port GPIOB

#define TWISTSW_TOP_Pin LL_GPIO_PIN_14
#define TWISTSW_TOP_GPIO_Port GPIOD
#define TWISTSW_BOT_Pin LL_GPIO_PIN_15
#define TWISTSW_BOT_GPIO_Port GPIOD

#define WARPSW_TOP_Pin LL_GPIO_PIN_15
#define WARPSW_TOP_GPIO_Port GPIOA
#define WARPSW_BOT_Pin LL_GPIO_PIN_10 
#define WARPSW_BOT_GPIO_Port GPIOC

#define FREEZE_BUT_Pin LL_GPIO_PIN_11
#define FREEZE_BUT_GPIO_Port GPIOA

#define LEARN_BUT_Pin LL_GPIO_PIN_9
#define LEARN_BUT_GPIO_Port GPIOC

// LEDs (TIM PWM)

#define LEARN_RED_Pin LL_GPIO_PIN_6
#define LEARN_RED_GPIO_Port GPIOC

#define LEARN_GREEN_Pin LL_GPIO_PIN_8
#define LEARN_GREEN_GPIO_Port GPIOC

#define LEARN_BLUE_Pin LL_GPIO_PIN_7
#define LEARN_BLUE_GPIO_Port GPIOC


#define FREEZE_RED_Pin LL_GPIO_PIN_8
#define FREEZE_RED_GPIO_Port GPIOA

#define FREEZE_GREEN_Pin LL_GPIO_PIN_10
#define FREEZE_GREEN_GPIO_Port GPIOA

#define FREEZE_BLUE_Pin LL_GPIO_PIN_9
#define FREEZE_BLUE_GPIO_Port GPIOA

// DEBUG header pins

#define DEBUG1_OUT_Pin LL_GPIO_PIN_5
#define DEBUG1_OUT_GPIO_Port GPIOD

#define DEBUG2_OUT_Pin LL_GPIO_PIN_6
#define DEBUG2_OUT_GPIO_Port GPIOD

#define DEBUG3_OUT_Pin LL_GPIO_PIN_7
#define DEBUG3_OUT_GPIO_Port GPIOD

#define DEBUG4_OUT_Pin LL_GPIO_PIN_4
#define DEBUG4_OUT_GPIO_Port GPIOD

#define DEBUG1_ON DEBUG1_OUT_GPIO_Port->BSRR = DEBUG1_OUT_Pin
#define DEBUG1_OFF DEBUG1_OUT_GPIO_Port->BSRR = (uint32_t)DEBUG1_OUT_Pin << 16

#define DEBUG2_ON DEBUG2_OUT_GPIO_Port->BSRR = DEBUG2_OUT_Pin
#define DEBUG2_OFF DEBUG2_OUT_GPIO_Port->BSRR = (uint32_t)DEBUG2_OUT_Pin << 16

#define DEBUG3_ON DEBUG3_OUT_GPIO_Port->BSRR = DEBUG3_OUT_Pin
#define DEBUG3_OFF DEBUG3_OUT_GPIO_Port->BSRR = (uint32_t)DEBUG3_OUT_Pin << 16

#define DEBUG4_ON DEBUG4_OUT_GPIO_Port->BSRR = DEBUG4_OUT_Pin
#define DEBUG4_OFF DEBUG4_OUT_GPIO_Port->BSRR = (uint32_t)DEBUG4_OUT_Pin << 16


//ADCs

#define SPREAD_CV_Pin LL_GPIO_PIN_1
#define SPREAD_CV_GPIO_Port GPIOA

#define WARP_CV_Pin LL_GPIO_PIN_2
#define WARP_CV_GPIO_Port GPIOA

#define CROSSFM_CV_Pin LL_GPIO_PIN_0
#define CROSSFM_CV_GPIO_Port GPIOC

#define CROSSFM_CV_2_Pin LL_GPIO_PIN_2
#define CROSSFM_CV_2_GPIO_Port GPIOC

#define TWIST_CV_Pin LL_GPIO_PIN_1
#define TWIST_CV_GPIO_Port GPIOC

#define BALANCE_CV_Pin LL_GPIO_PIN_3
#define BALANCE_CV_GPIO_Port GPIOC

#define SCALE_CV_Pin LL_GPIO_PIN_0
#define SCALE_CV_GPIO_Port GPIOA


#define WARP_POT_Pin LL_GPIO_PIN_3
#define WARP_POT_GPIO_Port GPIOA

#define DETUNE_POT_Pin LL_GPIO_PIN_4
#define DETUNE_POT_GPIO_Port GPIOA

#define CROSSFM_POT_Pin LL_GPIO_PIN_5
#define CROSSFM_POT_GPIO_Port GPIOA

#define ROOT_POT_Pin LL_GPIO_PIN_6
#define ROOT_POT_GPIO_Port GPIOA

#define SCALE_POT_Pin LL_GPIO_PIN_7
#define SCALE_POT_GPIO_Port GPIOA

#define PITCH_POT_Pin LL_GPIO_PIN_4
#define PITCH_POT_GPIO_Port GPIOC

#define SPREAD_POT_Pin LL_GPIO_PIN_5
#define SPREAD_POT_GPIO_Port GPIOC

#define BALANCE_POT_Pin LL_GPIO_PIN_0
#define BALANCE_POT_GPIO_Port GPIOB

#define TWIST_POT_Pin LL_GPIO_PIN_1
#define TWIST_POT_GPIO_Port GPIOB

#define ALL_GPIO_RCC_ENABLE     __HAL_RCC_GPIOA_CLK_ENABLE();\
                                __HAL_RCC_GPIOB_CLK_ENABLE();\
                                __HAL_RCC_GPIOC_CLK_ENABLE();\
                                __HAL_RCC_GPIOD_CLK_ENABLE();\
                                __HAL_RCC_GPIOE_CLK_ENABLE();\
                                __HAL_RCC_GPIOF_CLK_ENABLE();\
                                __HAL_RCC_GPIOG_CLK_ENABLE();\
                                __HAL_RCC_GPIOH_CLK_ENABLE();
                

#define FREEZE_LED_PWM_TIM          TIM1

#define FREEZE_LED_PWM_CC_RED       CCR1
#define FREEZE_LED_PWM_CC_GREEN     CCR3
#define FREEZE_LED_PWM_CC_BLUE      CCR2

#define FREEZE_LED_PWM_CHAN_RED     TIM_CHANNEL_1
#define FREEZE_LED_PWM_CHAN_GREEN   TIM_CHANNEL_3
#define FREEZE_LED_PWM_CHAN_BLUE    TIM_CHANNEL_2

#define FREEZE_LED_PWM_TIM_AF       GPIO_AF1_TIM1
#define FREEZE_LED_PWM_pins         (FREEZE_RED_Pin | FREEZE_GREEN_Pin | FREEZE_BLUE_Pin)
#define FREEZE_LED_PWM_GPIO         FREEZE_RED_GPIO_Port

#define LEARN_LED_PWM_TIM           TIM3

#define LEARN_LED_PWM_CC_RED        CCR1
#define LEARN_LED_PWM_CC_GREEN      CCR3
#define LEARN_LED_PWM_CC_BLUE       CCR2

#define LEARN_LED_PWM_CHAN_RED      TIM_CHANNEL_1
#define LEARN_LED_PWM_CHAN_GREEN    TIM_CHANNEL_3
#define LEARN_LED_PWM_CHAN_BLUE     TIM_CHANNEL_2

#define LEARN_LED_PWM_TIM_AF        GPIO_AF2_TIM3
#define LEARN_LED_PWM_pins          (LEARN_RED_Pin | LEARN_GREEN_Pin | LEARN_BLUE_Pin)
#define LEARN_LED_PWM_GPIO          LEARN_RED_GPIO_Port

void    init_gpio_pins(void);
