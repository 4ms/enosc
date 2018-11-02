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

//##############################################################################
// 								GPIOs and PINS 
//##############################################################################


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
