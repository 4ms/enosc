/*
 * codec_i2c.h: setup and init for WM8731 codec
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

//
// I2C Config
//
#define CODEC_I2C						I2C1
#define CODEC_I2C_CLK_ENABLE			__HAL_RCC_I2C1_CLK_ENABLE
#define CODEC_I2C_CLK_DISABLE			__HAL_RCC_I2C1_CLK_DISABLE
#define CODEC_I2C_GPIO_CLOCK_ENABLE		__HAL_RCC_GPIOB_CLK_ENABLE
#define CODEC_I2C_GPIO_AF				GPIO_AF4_I2C1
#define CODEC_I2C_GPIO					GPIOB
#define CODEC_I2C_SCL_PIN				GPIO_PIN_8
#define CODEC_I2C_SDA_PIN				GPIO_PIN_7


#define CODEC_FLAG_TIMEOUT             ((uint32_t)0x1000)
#define CODEC_SHORT_TIMEOUT            ((uint32_t)(1000))
#define CODEC_LONG_TIMEOUT             ((uint32_t)(300 * CODEC_FLAG_TIMEOUT))
#define CODEC_VLONG_TIMEOUT            ((uint32_t)(30000 * CODEC_FLAG_TIMEOUT))


void codec_deinit(void);
uint32_t codec_power_down(void);
uint32_t codec_register_setup(uint32_t sample_rate);
void codec_GPIO_init(void);
void codec_I2C_init(void);

