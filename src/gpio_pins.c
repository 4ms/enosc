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


#include "gpio_pins.h"
#include "globals.h"

void init_gpio_pins(void){
	GPIO_InitTypeDef gpio;

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

  // Debug header pins

	gpio.Pin = DEBUG1_OUT_Pin;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(DEBUG1_OUT_GPIO_Port, &gpio);

	gpio.Pin = DEBUG2_OUT_Pin;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(DEBUG2_OUT_GPIO_Port, &gpio);

	gpio.Pin = DEBUG3_OUT_Pin;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(DEBUG3_OUT_GPIO_Port, &gpio);

	gpio.Pin = DEBUG4_OUT_Pin;
	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(DEBUG4_OUT_GPIO_Port, &gpio);


	gpio.Mode = GPIO_MODE_OUTPUT_PP;
	gpio.Pull = GPIO_NOPULL;
	gpio.Speed = GPIO_SPEED_FREQ_HIGH;

	gpio.Pin = LEARN_RED_Pin;		HAL_GPIO_Init(LEARN_RED_GPIO_Port, &gpio);
	gpio.Pin = LEARN_GREEN_Pin;		HAL_GPIO_Init(LEARN_GREEN_GPIO_Port, &gpio);
	gpio.Pin = LEARN_BLUE_Pin;		HAL_GPIO_Init(LEARN_BLUE_GPIO_Port, &gpio);
	gpio.Pin = FREEZE_RED_Pin;		HAL_GPIO_Init(FREEZE_RED_GPIO_Port, &gpio);
	gpio.Pin = FREEZE_GREEN_Pin;	HAL_GPIO_Init(FREEZE_GREEN_GPIO_Port, &gpio);
	gpio.Pin = FREEZE_BLUE_Pin;		HAL_GPIO_Init(FREEZE_BLUE_GPIO_Port, &gpio);


}
