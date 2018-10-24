/*
 * main.c - PolyOsc test code
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

#include <stm32f7xx.h>
#include "globals.h"
#include "gpio_pins.h"
#include "drivers/codec_i2c.h"
#include "drivers/codec_sai.h"
#include "audio_stream.h"
#include "led_tim_pwm.h"
#include "adc_interface.h"
#include "hal_handlers.h"
//Private functions:
void SystemClock_Config(void);
void SetVectorTable(uint32_t reset_address);
void do_init(void);
extern uint16_t		builtin_adc1_raw[ NUM_BUILTIN_ADC1 ];
extern uint16_t		builtin_adc3_raw[ NUM_BUILTIN_ADC3 ];


////////////////////////////////////////////////////////////
// Below are the variables to watch:
///////////////////////////////////////////////////////////

enum GateStates 	freeze_jack, learn_jack;
enum ButtonStates 	learn_but, freeze_but;
enum SwitchStates 	mod_sw, grid_sw, twist_sw, warp_sw;
uint16_t warp_pot, detune_pot, mod_pot, root_pot, grid_pot, pitch_pot, spread_pot, tilt_pot, twist_pot;
uint16_t spread1_cv, warp_cv, spread2_cv, twist_cv, tilt_cv, grid_cv, mod_cv;



/////////////////////////////////////////////////////////////////////////
// Set USE_TIM_PWM_FOR_LEDS to 0 to use GPIO to drive the buttons
// ---pressing the buttons will cycle colors
//
// Set USE_TIM_PWM_FOR_LEDS to 1 to use TIM PWM driving the LED buttons
// ---turning 6 of the knobs will adjust the R/G/B values of each button
/////////////////////////////////////////////////////////////////////////

#define USE_TIM_PWM_FOR_LEDS 1



///////////////////////////////////////////////////////////
// Flip Mod Switch up or center to output test waveforms
// Flip down to pass-through audio (from Root and Pitch CV
///////////////////////////////////////////////////////////







uint32_t learn_but_armed=0, freeze_but_armed=0;
uint8_t learn_color=1, freeze_color=1;

int main(void)
{
	uint32_t last_update_tm = 0;

	do_init();

	while(1){
		///////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Step through this code line by line and verify the debug header pins and button LEDs are going high/low
		///////////////////////////////////////////////////////////////////////////////////////////////////////////

		//Debug header
		PIN_ON(DEBUG1_OUT_GPIO_Port, DEBUG1_OUT_Pin);
		PIN_OFF(DEBUG1_OUT_GPIO_Port, DEBUG1_OUT_Pin);

		PIN_ON(DEBUG2_OUT_GPIO_Port, DEBUG2_OUT_Pin);
		PIN_OFF(DEBUG2_OUT_GPIO_Port, DEBUG2_OUT_Pin);

		PIN_ON(DEBUG3_OUT_GPIO_Port, DEBUG3_OUT_Pin);
		PIN_OFF(DEBUG3_OUT_GPIO_Port, DEBUG3_OUT_Pin);

		PIN_ON(DEBUG4_OUT_GPIO_Port, DEBUG4_OUT_Pin);
		PIN_OFF(DEBUG4_OUT_GPIO_Port, DEBUG4_OUT_Pin);

		if (USE_TIM_PWM_FOR_LEDS)
		{
			//LEDs with PWM

			if ((HAL_GetTick() - last_update_tm) > 1000/60)
			{
				last_update_tm = HAL_GetTick();
				update_led_tim_pwm();
			}
		}
		else
		{
			//LEDs with GPIO

			//Change color when buttons are pressed
			//Simple de-bounce
			if (freeze_but==PRESSED) freeze_but_armed++;
			if (freeze_but==NOT_PRESSED)
			{
				if (freeze_but_armed>20000) freeze_color++; //reset_led(FREEZE_LED);}
				freeze_but_armed = 0;
			}

			if (freeze_color & 0b001) 	LED_ON(FREEZE_RED_GPIO_Port, FREEZE_RED_Pin);
			else						LED_OFF(FREEZE_RED_GPIO_Port, FREEZE_RED_Pin);

			if (freeze_color & 0b010) 	LED_ON(FREEZE_GREEN_GPIO_Port, FREEZE_GREEN_Pin);
			else						LED_OFF(FREEZE_GREEN_GPIO_Port, FREEZE_GREEN_Pin);

			if (freeze_color & 0b100) 	LED_ON(FREEZE_BLUE_GPIO_Port, FREEZE_BLUE_Pin);
			else						LED_OFF(FREEZE_BLUE_GPIO_Port, FREEZE_BLUE_Pin);

			if (learn_but==PRESSED) learn_but_armed++;
			if (learn_but==NOT_PRESSED)
			{
				if (learn_but_armed>20000) learn_color++; //reset_led(LEARN_LED);}
				learn_but_armed = 0;
			}

			if (learn_color & 0b001) 	LED_ON(LEARN_RED_GPIO_Port, LEARN_RED_Pin);
			else						LED_OFF(LEARN_RED_GPIO_Port, LEARN_RED_Pin);

			if (learn_color & 0b010) 	LED_ON(LEARN_GREEN_GPIO_Port, LEARN_GREEN_Pin);
			else						LED_OFF(LEARN_GREEN_GPIO_Port, LEARN_GREEN_Pin);

			if (learn_color & 0b100) 	LED_ON(LEARN_BLUE_GPIO_Port, LEARN_BLUE_Pin);
			else						LED_OFF(LEARN_BLUE_GPIO_Port, LEARN_BLUE_Pin);
		}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Run the code at normal speed and watch the variables in the debugger. 
		// The appropriate variables should respond to incoming gates/CV and turning the pots
		///////////////////////////////////////////////////////////////////////////////////////////////////////////

		//Gate jacks
		freeze_jack = PIN_READ(FREEZE_JACK_GPIO_Port, FREEZE_JACK_Pin) ? JACK_HIGH : JACK_LOW;
		learn_jack = PIN_READ(LEARN_JACK_GPIO_Port, LEARN_JACK_Pin) ? JACK_HIGH : JACK_LOW;

		//Buttons
		freeze_but = PIN_READ(FREEZE_BUT_GPIO_Port, FREEZE_BUT_Pin) ? NOT_PRESSED : PRESSED;
		learn_but = PIN_READ(LEARN_BUT_GPIO_Port, LEARN_BUT_Pin) ? NOT_PRESSED : PRESSED;

		//MOD Switch
		if (PIN_READ(MODSW_TOP_GPIO_Port, MODSW_TOP_Pin))
		{
			if (PIN_READ(MODSW_BOT_GPIO_Port, MODSW_BOT_Pin))
				mod_sw = SWITCH_CENTER;	//top and bottom pins high
			else
				mod_sw = SWITCH_DOWN; //top high, bottom low
		}
		else {
			if (PIN_READ(MODSW_BOT_GPIO_Port, MODSW_BOT_Pin))
				mod_sw = SWITCH_UP;	//top low, bottom high
			else
				mod_sw = SWITCH_INVALID; //top low, bottom low
		}

		//GRID Switch
		if (PIN_READ(GRIDSW_TOP_GPIO_Port, GRIDSW_TOP_Pin))
		{
			if (PIN_READ(GRIDSW_BOT_GPIO_Port, GRIDSW_BOT_Pin))
				grid_sw = SWITCH_CENTER;	//top and bottom pins high
			else
				grid_sw = SWITCH_DOWN; //top high, bottom low
		}
		else {
			if (PIN_READ(GRIDSW_BOT_GPIO_Port, GRIDSW_BOT_Pin))
				grid_sw = SWITCH_UP;	//top low, bottom high
			else
				grid_sw = SWITCH_INVALID; //top low, bottom low
		}

		//TWIST Switch
		if (PIN_READ(TWISTSW_TOP_GPIO_Port, TWISTSW_TOP_Pin))
		{
			if (PIN_READ(TWISTSW_BOT_GPIO_Port, TWISTSW_BOT_Pin))
				twist_sw = SWITCH_CENTER;	//top and bottom pins high
			else
				twist_sw = SWITCH_DOWN; //top high, bottom low
		}
		else {
			if (PIN_READ(TWISTSW_BOT_GPIO_Port, TWISTSW_BOT_Pin))
				twist_sw = SWITCH_UP;	//top low, bottom high
			else
				twist_sw = SWITCH_INVALID; //top low, bottom low
		}

		//WARP Switch
		if (PIN_READ(WARPSW_TOP_GPIO_Port, WARPSW_TOP_Pin))
		{
			if (PIN_READ(WARPSW_BOT_GPIO_Port, WARPSW_BOT_Pin))
				warp_sw = SWITCH_CENTER;	//top and bottom pins high
			else
				warp_sw = SWITCH_DOWN; //top high, bottom low
		}
		else {
			if (PIN_READ(WARPSW_BOT_GPIO_Port, WARPSW_BOT_Pin))
				warp_sw = SWITCH_UP;	//top low, bottom high
			else
				warp_sw = SWITCH_INVALID; //top low, bottom low
		}

		//ADCs
		warp_pot = builtin_adc1_raw[WARP_POT_ADC];
		detune_pot = builtin_adc1_raw[DETUNE_POT_ADC];
		mod_pot = builtin_adc1_raw[MOD_POT_ADC];
		root_pot = builtin_adc1_raw[ROOT_POT_ADC];
		grid_pot = builtin_adc1_raw[GRID_POT_ADC];
		pitch_pot = builtin_adc1_raw[PITCH_POT_ADC];
		spread_pot = builtin_adc1_raw[SPREAD_POT_ADC];
		tilt_pot = builtin_adc1_raw[TILT_POT_ADC];
		twist_pot = builtin_adc1_raw[TWIST_POT_ADC];

		spread1_cv = builtin_adc3_raw[SPREAD_CV_1_ADC];
		warp_cv = builtin_adc3_raw[WARP_CV_ADC];
		spread2_cv = builtin_adc3_raw[SPREAD_CV_2_ADC];
		twist_cv = builtin_adc3_raw[TWIST_CV_ADC];
		tilt_cv = builtin_adc3_raw[TILT_CV_ADC];
		grid_cv = builtin_adc3_raw[GRID_CV_ADC];
		mod_cv = builtin_adc3_raw[MOD_CV_ADC];

	} //end main loop

	return(0);

} //end main()


void do_init(void)
{

	uint32_t err;

	SetVectorTable(0x08000000);

	HAL_Init();
	SystemClock_Config();

	// SCB_EnableICache();
	SCB_DisableICache(); //not needed because we're running from FLASH on the ITCM bus, using ART and Prefetch

	SCB_InvalidateDCache();
	SCB_EnableDCache();	
	//SCB_DisableDCache();	

	__HAL_RCC_DMA2_CLK_DISABLE();

	// Setup PLL clock for codec
   	init_SAI_clock(SAMPLERATE);

   	//De-init the codec to force it to reset
    codec_deinit();
	HAL_Delay(10);

	// INITIALIZATIONS
	init_gpio_pins();
	HAL_Delay(10);
	
	#if (USE_TIM_PWM_FOR_LEDS)
	//Setup PWM LED pins
	init_led_tim_pwm();
	#endif

	// Init ADC
	adc_init_all();
	HAL_Delay(100);

	//Start Codec I2C
	codec_GPIO_init();
	codec_I2C_init();
	
	HAL_Delay(100);
	err = codec_register_setup(SAMPLERATE);
	if (err){_Error_Handler(__FILE__, __LINE__);}

	//Start Codec SAI
	codec_SAI_init(SAMPLERATE);
	init_audio_DMA();

	//Start audio processing
	start_audio();
}


void SystemClock_Config(void)
{

	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

	//Configure the main internal regulator output voltage 
	__HAL_RCC_PWR_CLK_ENABLE();

	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	//Initializes the CPU, AHB and APB busses clocks 
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 16;
	RCC_OscInitStruct.PLL.PLLN = 432;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
		_Error_Handler(__FILE__, __LINE__);

	//Activate the OverDrive to reach the 216 MHz Frequency 
	if (HAL_PWREx_EnableOverDrive() != HAL_OK)
		_Error_Handler(__FILE__, __LINE__);

	//Initializes the CPU, AHB and APB busses clocks 
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
		_Error_Handler(__FILE__, __LINE__);


	//Note: Do not start the SAI clock (I2S) at this time. 
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C1;

	PeriphClkInitStruct.I2c1ClockSelection 		= RCC_I2C1CLKSOURCE_PCLK1; //54MHz

	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		_Error_Handler(__FILE__, __LINE__);
	}

	//Enables the Clock Security System 
	HAL_RCC_EnableCSS();

	// Configure the Systick interrupt time for 1ms
	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

	//Configure the Systick 
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

	// SysTick_IRQn interrupt configuration 
	HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

void SetVectorTable(uint32_t reset_address)
{ 
	SCB->VTOR = reset_address & (uint32_t)0x1FFFFF80;
}


//
// SysTick_Handler() is needed for HAL_GetTick()
//
void SysTick_Handler(void)
{
	HAL_IncTick();
	HAL_SYSTICK_IRQHandler();
}



