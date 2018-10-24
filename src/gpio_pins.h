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

#define PIN_ON(x,y) 	x->BSRR = y
#define PIN_OFF(x,y) 	x->BSRR = (uint32_t)y << 16
#define PIN_READ(x, y) (((x->IDR) & y) ? 1 : 0)

#define LED_ON(x,y) 	PIN_OFF(x,y)
#define LED_OFF(x,y) 	PIN_ON(x,y)

//##############################################################################
// 								GPIOs and PINS 
//##############################################################################

// Gate jacks

#define FREEZE_JACK_Pin GPIO_PIN_2
#define FREEZE_JACK_GPIO_Port GPIOB

#define LEARN_JACK_Pin GPIO_PIN_7
#define LEARN_JACK_GPIO_Port GPIOE


//Switches/Buttons

#define MODSW_TOP_Pin GPIO_PIN_14
#define MODSW_TOP_GPIO_Port GPIOE
#define MODSW_BOT_Pin GPIO_PIN_15
#define MODSW_BOT_GPIO_Port GPIOE

#define GRIDSW_TOP_Pin GPIO_PIN_12
#define GRIDSW_TOP_GPIO_Port GPIOB
#define GRIDSW_BOT_Pin GPIO_PIN_13
#define GRIDSW_BOT_GPIO_Port GPIOB

#define TWISTSW_TOP_Pin GPIO_PIN_14
#define TWISTSW_TOP_GPIO_Port GPIOD
#define TWISTSW_BOT_Pin GPIO_PIN_15
#define TWISTSW_BOT_GPIO_Port GPIOD

#define WARPSW_TOP_Pin GPIO_PIN_11 /*reversed from PCB*/
#define WARPSW_TOP_GPIO_Port GPIOC
#define WARPSW_BOT_Pin GPIO_PIN_10 /*reversed from PCB*/
#define WARPSW_BOT_GPIO_Port GPIOC

// LEDs (TIM PWM)

#define LEARN_RED_Pin GPIO_PIN_6
#define LEARN_RED_GPIO_Port GPIOC

#define LEARN_GREEN_Pin GPIO_PIN_8
#define LEARN_GREEN_GPIO_Port GPIOC

#define LEARN_BLUE_Pin GPIO_PIN_7
#define LEARN_BLUE_GPIO_Port GPIOC


#define FREEZE_RED_Pin GPIO_PIN_8
#define FREEZE_RED_GPIO_Port GPIOA

#define FREEZE_GREEN_Pin GPIO_PIN_10
#define FREEZE_GREEN_GPIO_Port GPIOA

#define FREEZE_BLUE_Pin GPIO_PIN_9
#define FREEZE_BLUE_GPIO_Port GPIOA

// DEBUG header pins

#define DEBUG1_OUT_Pin GPIO_PIN_5
#define DEBUG1_OUT_GPIO_Port GPIOD

#define DEBUG2_OUT_Pin GPIO_PIN_6
#define DEBUG2_OUT_GPIO_Port GPIOD

#define DEBUG3_OUT_Pin GPIO_PIN_7
#define DEBUG3_OUT_GPIO_Port GPIOD

#define DEBUG4_OUT_Pin GPIO_PIN_4
#define DEBUG4_OUT_GPIO_Port GPIOD

//ADCs

#define SPREAD_CV_1_Pin GPIO_PIN_0
#define SPREAD_CV_1_GPIO_Port GPIOC

#define WARP_CV_Pin GPIO_PIN_1
#define WARP_CV_GPIO_Port GPIOC

#define SPREAD_CV_2_Pin GPIO_PIN_2
#define SPREAD_CV_2_GPIO_Port GPIOC

#define TWIST_CV_Pin GPIO_PIN_3
#define TWIST_CV_GPIO_Port GPIOC

#define TILT_CV_Pin GPIO_PIN_0
#define TILT_CV_GPIO_Port GPIOA

#define GRID_CV_Pin GPIO_PIN_1
#define GRID_CV_GPIO_Port GPIOA

#define MOD_CV_Pin GPIO_PIN_2
#define MOD_CV_GPIO_Port GPIOA

#define WARP_POT_Pin GPIO_PIN_3
#define WARP_POT_GPIO_Port GPIOA

#define DETUNE_POT_Pin GPIO_PIN_4
#define DETUNE_POT_GPIO_Port GPIOA

#define MOD_POT_Pin GPIO_PIN_5
#define MOD_POT_GPIO_Port GPIOA

#define ROOT_POT_Pin GPIO_PIN_6
#define ROOT_POT_GPIO_Port GPIOA

#define GRID_POT_Pin GPIO_PIN_7
#define GRID_POT_GPIO_Port GPIOA

#define PITCH_POT_Pin GPIO_PIN_4
#define PITCH_POT_GPIO_Port GPIOC

#define SPREAD_POT_Pin GPIO_PIN_5
#define SPREAD_POT_GPIO_Port GPIOC

#define TILT_POT_Pin GPIO_PIN_0
#define TILT_POT_GPIO_Port GPIOB

#define TWIST_POT_Pin GPIO_PIN_1
#define TWIST_POT_GPIO_Port GPIOB

void 	init_gpio_pins(void);
