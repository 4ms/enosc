/*
 * adc_interface.c - Combines multiple ADC interfaces (internal peripherals, external chips)
 *                   into one single analog[] structure
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

#include "adc_interface.h"
#include "gpio_pins.h"
#include "drivers/adc_builtin_driver.h"

/*
 * How to Use:
 * 
 * Requires: adc_builtin_driver.c/h
 *
 * In adc_interface.h:
 *   - Set the enum elements to the names of the ADCs you want to read.
 *
 * In this file (adc_interface.c):
 *   - Set the pins, GPIOs, channel, and sample_time for each built-in ADC in the builtin_adc_setup() function
 *
 * Then, call adc_init_all() in your main application
 */


builtinAdcSetup	adc1_setup[NUM_BUILTIN_ADC1];
builtinAdcSetup	adc3_setup[NUM_BUILTIN_ADC3];

void builtin_adc_setup(void)
{
	//Setup and initialize the builtin ADCSs

	adc1_setup[WARP_POT_ADC].gpio 			= WARP_POT_GPIO_Port;
	adc1_setup[WARP_POT_ADC].pin 			= WARP_POT_Pin;
	adc1_setup[WARP_POT_ADC].channel 		= ADC_CHANNEL_3;
	adc1_setup[WARP_POT_ADC].sample_time 	= ADC_SAMPLETIME_480CYCLES;

	adc1_setup[DETUNE_POT_ADC].gpio 		= DETUNE_POT_GPIO_Port;
	adc1_setup[DETUNE_POT_ADC].pin 			= DETUNE_POT_Pin;
	adc1_setup[DETUNE_POT_ADC].channel 		= ADC_CHANNEL_4;
	adc1_setup[DETUNE_POT_ADC].sample_time 	= ADC_SAMPLETIME_480CYCLES;

	adc1_setup[MOD_POT_ADC].gpio 			= MOD_POT_GPIO_Port;
	adc1_setup[MOD_POT_ADC].pin 			= MOD_POT_Pin;
	adc1_setup[MOD_POT_ADC].channel 		= ADC_CHANNEL_5;
	adc1_setup[MOD_POT_ADC].sample_time 	= ADC_SAMPLETIME_480CYCLES;

	adc1_setup[ROOT_POT_ADC].gpio 			= ROOT_POT_GPIO_Port;
	adc1_setup[ROOT_POT_ADC].pin 			= ROOT_POT_Pin;
	adc1_setup[ROOT_POT_ADC].channel 		= ADC_CHANNEL_6;
	adc1_setup[ROOT_POT_ADC].sample_time 	= ADC_SAMPLETIME_480CYCLES;

	adc1_setup[GRID_POT_ADC].gpio 			= GRID_POT_GPIO_Port;
	adc1_setup[GRID_POT_ADC].pin 			= GRID_POT_Pin;
	adc1_setup[GRID_POT_ADC].channel 		= ADC_CHANNEL_7;
	adc1_setup[GRID_POT_ADC].sample_time 	= ADC_SAMPLETIME_480CYCLES;

	adc1_setup[PITCH_POT_ADC].gpio 			= PITCH_POT_GPIO_Port;
	adc1_setup[PITCH_POT_ADC].pin 			= PITCH_POT_Pin;
	adc1_setup[PITCH_POT_ADC].channel 		= ADC_CHANNEL_14;
	adc1_setup[PITCH_POT_ADC].sample_time 	= ADC_SAMPLETIME_480CYCLES;

	adc1_setup[SPREAD_POT_ADC].gpio 		= SPREAD_POT_GPIO_Port;
	adc1_setup[SPREAD_POT_ADC].pin 			= SPREAD_POT_Pin;
	adc1_setup[SPREAD_POT_ADC].channel 		= ADC_CHANNEL_15;
	adc1_setup[SPREAD_POT_ADC].sample_time	= ADC_SAMPLETIME_480CYCLES;

	adc1_setup[TILT_POT_ADC].gpio 			= TILT_POT_GPIO_Port;
	adc1_setup[TILT_POT_ADC].pin 			= TILT_POT_Pin;
	adc1_setup[TILT_POT_ADC].channel 		= ADC_CHANNEL_8;
	adc1_setup[TILT_POT_ADC].sample_time 	= ADC_SAMPLETIME_480CYCLES;

	adc1_setup[TWIST_POT_ADC].gpio 			= TWIST_POT_GPIO_Port;
	adc1_setup[TWIST_POT_ADC].pin 			= TWIST_POT_Pin;
	adc1_setup[TWIST_POT_ADC].channel 		= ADC_CHANNEL_9;
	adc1_setup[TWIST_POT_ADC].sample_time 	= ADC_SAMPLETIME_480CYCLES;

	//ADC3:
	adc3_setup[SPREAD_CV_1_ADC].gpio 		= SPREAD_CV_1_GPIO_Port;
	adc3_setup[SPREAD_CV_1_ADC].pin 		= SPREAD_CV_1_Pin;
	adc3_setup[SPREAD_CV_1_ADC].channel 	= ADC_CHANNEL_10;
	adc3_setup[SPREAD_CV_1_ADC].sample_time = ADC_SAMPLETIME_480CYCLES;

	adc3_setup[WARP_CV_ADC].gpio 			= WARP_CV_GPIO_Port;
	adc3_setup[WARP_CV_ADC].pin 			= WARP_CV_Pin;
	adc3_setup[WARP_CV_ADC].channel 		= ADC_CHANNEL_11;
	adc3_setup[WARP_CV_ADC].sample_time 	= ADC_SAMPLETIME_480CYCLES;

	adc3_setup[SPREAD_CV_2_ADC].gpio 		= SPREAD_CV_2_GPIO_Port;
	adc3_setup[SPREAD_CV_2_ADC].pin 		= SPREAD_CV_2_Pin;
	adc3_setup[SPREAD_CV_2_ADC].channel 	= ADC_CHANNEL_12;
	adc3_setup[SPREAD_CV_2_ADC].sample_time = ADC_SAMPLETIME_480CYCLES;

	adc3_setup[TWIST_CV_ADC].gpio 			= TWIST_CV_GPIO_Port;
	adc3_setup[TWIST_CV_ADC].pin 			= TWIST_CV_Pin;
	adc3_setup[TWIST_CV_ADC].channel 		= ADC_CHANNEL_13;
	adc3_setup[TWIST_CV_ADC].sample_time 	= ADC_SAMPLETIME_480CYCLES;

	adc3_setup[TILT_CV_ADC].gpio 			= TILT_CV_GPIO_Port;
	adc3_setup[TILT_CV_ADC].pin 			= TILT_CV_Pin;
	adc3_setup[TILT_CV_ADC].channel 		= ADC_CHANNEL_0;
	adc3_setup[TILT_CV_ADC].sample_time 	= ADC_SAMPLETIME_480CYCLES;

	adc3_setup[GRID_CV_ADC].gpio 			= GRID_CV_GPIO_Port;
	adc3_setup[GRID_CV_ADC].pin 			= GRID_CV_Pin;
	adc3_setup[GRID_CV_ADC].channel 		= ADC_CHANNEL_1;
	adc3_setup[GRID_CV_ADC].sample_time 	= ADC_SAMPLETIME_480CYCLES;

	adc3_setup[MOD_CV_ADC].gpio 			= MOD_CV_GPIO_Port;
	adc3_setup[MOD_CV_ADC].pin 				= MOD_CV_Pin;
	adc3_setup[MOD_CV_ADC].channel 			= ADC_CHANNEL_2;
	adc3_setup[MOD_CV_ADC].sample_time 		= ADC_SAMPLETIME_480CYCLES;

}


//
// *_adc_raw[] is where raw adc data is stored
//
// extern these into any file that needs to read the adcs

uint16_t	builtin_adc1_raw[ NUM_BUILTIN_ADC1 ];
uint16_t	builtin_adc3_raw[ NUM_BUILTIN_ADC3 ];


//
// adc_init_all()
//
// Initialize all ADCs, and assigns their values to raw_adc_array
//
void adc_init_all(void)
{
	//populate the builtinAdcSetup arrays
	builtin_adc_setup();

	//Initialize and start the ADC and DMA
	ADC1_Init(builtin_adc1_raw, NUM_BUILTIN_ADC1, adc1_setup);
	ADC3_Init(builtin_adc3_raw, NUM_BUILTIN_ADC3, adc3_setup);

}
