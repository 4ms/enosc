/*
 * adc_builtin_driver.h - adc driver for built-in adcs
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

typedef struct builtinAdcSetup{
        GPIO_TypeDef *gpio;
        uint16_t pin;
        uint32_t channel;
        // uint32_t sample_time; //must be a valid ADC_SAMPLETIME_XXXCYCLES
} builtinAdcSetup;


// ADC1
enum BuiltinAdcChannels{
        WARP_POT_ADC,           //  0
        DETUNE_POT_ADC,         //  1
        CROSSFM_POT_ADC,        //  2
        ROOT_POT_ADC,           //  3
        SCALE_POT_ADC,          //  4
        PITCH_POT_ADC,          //  5
        SPREAD_POT_ADC,         //  6
        BALANCE_POT_ADC,        //  7
        TWIST_POT_ADC,          //  8

        SPREAD_CV_ADC,          //  9
        WARP_CV_ADC,            //  10 
        CROSSFM_CV_2_ADC,       //  11
        TWIST_CV_ADC,           //  12
        BALANCE_CV_ADC,         //  13
        SCALE_CV_ADC,           //  14
        CROSSFM_CV_ADC,         //  15
        
        NUM_ADCS
};

#define NUM_POT_ADC1 9
#define NUM_CV_ADC3 (NUM_ADCS - NUM_POT_ADC1)

void adc_init_all(void);
uint16_t read_adc(ADC_TypeDef *adc, uint32_t channel);
void init_adcs(void);
