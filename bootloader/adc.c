/*
 * adc_builtin_driver.c - adc driver for built-in adcs
 * Uses DMA to dump ADC values into buffers
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
#include "adc.h"
#include "lib/stm32f7xx_ll_adc.h"

builtinAdcSetup adc_[NUM_ADCS];
void builtin_adc_setup(void)
{
    //Setup and initialize the builtin ADCSs

    adc_[WARP_POT_ADC].gpio           = WARP_POT_GPIO_Port;
    adc_[WARP_POT_ADC].pin            = WARP_POT_Pin;
    adc_[WARP_POT_ADC].channel        = LL_ADC_CHANNEL_3; //or: ADC_CHANNEL_3_NUMBER, or just: 3

    adc_[DETUNE_POT_ADC].gpio         = DETUNE_POT_GPIO_Port;
    adc_[DETUNE_POT_ADC].pin          = DETUNE_POT_Pin;
    adc_[DETUNE_POT_ADC].channel      = LL_ADC_CHANNEL_4;

    adc_[CROSSFM_POT_ADC].gpio        = CROSSFM_POT_GPIO_Port;
    adc_[CROSSFM_POT_ADC].pin         = CROSSFM_POT_Pin;
    adc_[CROSSFM_POT_ADC].channel     = LL_ADC_CHANNEL_5;

    adc_[ROOT_POT_ADC].gpio           = ROOT_POT_GPIO_Port;
    adc_[ROOT_POT_ADC].pin            = ROOT_POT_Pin;
    adc_[ROOT_POT_ADC].channel        = LL_ADC_CHANNEL_6;

    adc_[SCALE_POT_ADC].gpio          = SCALE_POT_GPIO_Port;
    adc_[SCALE_POT_ADC].pin           = SCALE_POT_Pin;
    adc_[SCALE_POT_ADC].channel       = LL_ADC_CHANNEL_7;

    adc_[PITCH_POT_ADC].gpio          = PITCH_POT_GPIO_Port;
    adc_[PITCH_POT_ADC].pin           = PITCH_POT_Pin;
    adc_[PITCH_POT_ADC].channel       = LL_ADC_CHANNEL_14;

    adc_[SPREAD_POT_ADC].gpio         = SPREAD_POT_GPIO_Port;
    adc_[SPREAD_POT_ADC].pin          = SPREAD_POT_Pin;
    adc_[SPREAD_POT_ADC].channel      = LL_ADC_CHANNEL_15;

    adc_[BALANCE_POT_ADC].gpio        = BALANCE_POT_GPIO_Port;
    adc_[BALANCE_POT_ADC].pin         = BALANCE_POT_Pin;
    adc_[BALANCE_POT_ADC].channel     = LL_ADC_CHANNEL_8;

    adc_[TWIST_POT_ADC].gpio          = TWIST_POT_GPIO_Port;
    adc_[TWIST_POT_ADC].pin           = TWIST_POT_Pin;
    adc_[TWIST_POT_ADC].channel       = LL_ADC_CHANNEL_9;

    //ADC3:
    adc_[SPREAD_CV_ADC].gpio      = SPREAD_CV_GPIO_Port;
    adc_[SPREAD_CV_ADC].pin       = SPREAD_CV_Pin;
    adc_[SPREAD_CV_ADC].channel   = LL_ADC_CHANNEL_1; //10

    adc_[WARP_CV_ADC].gpio            = WARP_CV_GPIO_Port;
    adc_[WARP_CV_ADC].pin             = WARP_CV_Pin;
    adc_[WARP_CV_ADC].channel         = LL_ADC_CHANNEL_2; //11

    adc_[CROSSFM_CV_ADC].gpio         = CROSSFM_CV_GPIO_Port;
    adc_[CROSSFM_CV_ADC].pin          = CROSSFM_CV_Pin;
    adc_[CROSSFM_CV_ADC].channel      = LL_ADC_CHANNEL_10; //2

    adc_[CROSSFM_CV_2_ADC].gpio       = CROSSFM_CV_2_GPIO_Port;
    adc_[CROSSFM_CV_2_ADC].pin        = CROSSFM_CV_2_Pin;
    adc_[CROSSFM_CV_2_ADC].channel    = LL_ADC_CHANNEL_12;

    adc_[TWIST_CV_ADC].gpio           = TWIST_CV_GPIO_Port;
    adc_[TWIST_CV_ADC].pin            = TWIST_CV_Pin;
    adc_[TWIST_CV_ADC].channel        = LL_ADC_CHANNEL_11; //13

    adc_[BALANCE_CV_ADC].gpio         = BALANCE_CV_GPIO_Port;
    adc_[BALANCE_CV_ADC].pin          = BALANCE_CV_Pin;
    adc_[BALANCE_CV_ADC].channel      = LL_ADC_CHANNEL_13; //0 

    adc_[SCALE_CV_ADC].gpio           = SCALE_CV_GPIO_Port;
    adc_[SCALE_CV_ADC].pin            = SCALE_CV_Pin;
    adc_[SCALE_CV_ADC].channel        = LL_ADC_CHANNEL_0; //1
}

void adc_init_all(void)
{
    builtin_adc_setup();
    init_adcs();
}


uint16_t read_adc(ADC_TypeDef *adc, uint32_t channel) {
    adc->SQR3 = adc_[channel].channel;
    LL_ADC_SetChannelSamplingTime(adc, channel, LL_ADC_SAMPLINGTIME_480CYCLES);
    LL_ADC_REG_StartConversionSWStart(adc);
    while (LL_ADC_IsActiveFlag_EOCS(adc) == 0) {;}
    // LL_ADC_ClearFlag_EOCS(ADC1);
    return adc->DR;
}

void init_adcs(void)
{
    uint8_t i;
    GPIO_InitTypeDef gpio;
    ADC_TypeDef *adc;

    //Note: GPIO RCC should be enabled by now
    // LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);

    //Enable GPIO pins (and channel sampling time) Todo: is it OK to set the channel sampling time here?
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    for (i=0; i<NUM_ADCS; i++)
    {
        gpio.Pin = adc_[i].pin;
        HAL_GPIO_Init(adc_[i].gpio, &gpio);
    //  LL_GPIO_SetPinMode(adc_[i].gpio, adc_[i].pin, LL_GPIO_MODE_ANALOG);
    }

    //Enable ADC core clock
    SET_BIT(RCC->APB2ENR, RCC_APB2ENR_ADC1EN);
    SET_BIT(RCC->APB2ENR, RCC_APB2ENR_ADC3EN);
    // LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_ADC1);
    // LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_ADC3);

    LL_ADC_SetCommonClock(ADC123_COMMON, LL_ADC_CLOCK_SYNC_PCLK_DIV2);

    LL_ADC_REG_SetTriggerSource(ADC1, LL_ADC_REG_TRIG_SOFTWARE);
    LL_ADC_REG_SetContinuousMode(ADC1, LL_ADC_REG_CONV_SINGLE);
    LL_ADC_REG_SetFlagEndOfConversion(ADC1, LL_ADC_REG_FLAG_EOC_UNITARY_CONV);
    LL_ADC_REG_SetSequencerLength(ADC1, LL_ADC_REG_SEQ_SCAN_DISABLE);
    LL_ADC_Enable(ADC1);

    LL_ADC_REG_SetTriggerSource(ADC3, LL_ADC_REG_TRIG_SOFTWARE);
    LL_ADC_REG_SetContinuousMode(ADC3, LL_ADC_REG_CONV_SINGLE);
    LL_ADC_REG_SetFlagEndOfConversion(ADC3, LL_ADC_REG_FLAG_EOC_UNITARY_CONV);
    LL_ADC_REG_SetSequencerLength(ADC3, LL_ADC_REG_SEQ_SCAN_DISABLE);
    LL_ADC_Enable(ADC3);
}
