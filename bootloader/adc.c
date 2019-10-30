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

// ADC_HandleTypeDef hadc[2];
// DMA_HandleTypeDef hdma_adc[2];

builtinAdcSetup adc1_setup[NUM_BUILTIN_ADC1];
builtinAdcSetup adc3_setup[NUM_BUILTIN_ADC3];

void builtin_adc_setup(void)
{
    //Setup and initialize the builtin ADCSs

    adc1_setup[WARP_POT_ADC].gpio           = WARP_POT_GPIO_Port;
    adc1_setup[WARP_POT_ADC].pin            = WARP_POT_Pin;
    adc1_setup[WARP_POT_ADC].channel        = LL_ADC_CHANNEL_3;
    adc1_setup[WARP_POT_ADC].sample_time    = LL_ADC_SAMPLINGTIME_480CYCLES;

    adc1_setup[DETUNE_POT_ADC].gpio         = DETUNE_POT_GPIO_Port;
    adc1_setup[DETUNE_POT_ADC].pin          = DETUNE_POT_Pin;
    adc1_setup[DETUNE_POT_ADC].channel      = LL_ADC_CHANNEL_4;
    adc1_setup[DETUNE_POT_ADC].sample_time  = LL_ADC_SAMPLINGTIME_480CYCLES;

    adc1_setup[CROSSFM_POT_ADC].gpio        = CROSSFM_POT_GPIO_Port;
    adc1_setup[CROSSFM_POT_ADC].pin         = CROSSFM_POT_Pin;
    adc1_setup[CROSSFM_POT_ADC].channel     = LL_ADC_CHANNEL_5;
    adc1_setup[CROSSFM_POT_ADC].sample_time = LL_ADC_SAMPLINGTIME_480CYCLES;

    adc1_setup[ROOT_POT_ADC].gpio           = ROOT_POT_GPIO_Port;
    adc1_setup[ROOT_POT_ADC].pin            = ROOT_POT_Pin;
    adc1_setup[ROOT_POT_ADC].channel        = LL_ADC_CHANNEL_6;
    adc1_setup[ROOT_POT_ADC].sample_time    = LL_ADC_SAMPLINGTIME_480CYCLES;

    adc1_setup[SCALE_POT_ADC].gpio          = SCALE_POT_GPIO_Port;
    adc1_setup[SCALE_POT_ADC].pin           = SCALE_POT_Pin;
    adc1_setup[SCALE_POT_ADC].channel       = LL_ADC_CHANNEL_7;
    adc1_setup[SCALE_POT_ADC].sample_time   = LL_ADC_SAMPLINGTIME_480CYCLES;

    adc1_setup[PITCH_POT_ADC].gpio          = PITCH_POT_GPIO_Port;
    adc1_setup[PITCH_POT_ADC].pin           = PITCH_POT_Pin;
    adc1_setup[PITCH_POT_ADC].channel       = LL_ADC_CHANNEL_14;
    adc1_setup[PITCH_POT_ADC].sample_time   = LL_ADC_SAMPLINGTIME_480CYCLES;

    adc1_setup[SPREAD_POT_ADC].gpio         = SPREAD_POT_GPIO_Port;
    adc1_setup[SPREAD_POT_ADC].pin          = SPREAD_POT_Pin;
    adc1_setup[SPREAD_POT_ADC].channel      = LL_ADC_CHANNEL_15;
    adc1_setup[SPREAD_POT_ADC].sample_time  = LL_ADC_SAMPLINGTIME_480CYCLES;

    adc1_setup[BALANCE_POT_ADC].gpio        = BALANCE_POT_GPIO_Port;
    adc1_setup[BALANCE_POT_ADC].pin         = BALANCE_POT_Pin;
    adc1_setup[BALANCE_POT_ADC].channel     = LL_ADC_CHANNEL_8;
    adc1_setup[BALANCE_POT_ADC].sample_time = LL_ADC_SAMPLINGTIME_480CYCLES;

    adc1_setup[TWIST_POT_ADC].gpio          = TWIST_POT_GPIO_Port;
    adc1_setup[TWIST_POT_ADC].pin           = TWIST_POT_Pin;
    adc1_setup[TWIST_POT_ADC].channel       = LL_ADC_CHANNEL_9;
    adc1_setup[TWIST_POT_ADC].sample_time   = LL_ADC_SAMPLINGTIME_480CYCLES;

    //ADC3:
    adc3_setup[SPREAD_CV_ADC].gpio      = SPREAD_CV_GPIO_Port;
    adc3_setup[SPREAD_CV_ADC].pin       = SPREAD_CV_Pin;
    adc3_setup[SPREAD_CV_ADC].channel   = LL_ADC_CHANNEL_1; //10
    adc3_setup[SPREAD_CV_ADC].sample_time = LL_ADC_SAMPLINGTIME_480CYCLES;

    adc3_setup[WARP_CV_ADC].gpio            = WARP_CV_GPIO_Port;
    adc3_setup[WARP_CV_ADC].pin             = WARP_CV_Pin;
    adc3_setup[WARP_CV_ADC].channel         = LL_ADC_CHANNEL_2; //11
    adc3_setup[WARP_CV_ADC].sample_time     = LL_ADC_SAMPLINGTIME_480CYCLES;

    adc3_setup[CROSSFM_CV_ADC].gpio         = CROSSFM_CV_GPIO_Port;
    adc3_setup[CROSSFM_CV_ADC].pin          = CROSSFM_CV_Pin;
    adc3_setup[CROSSFM_CV_ADC].channel      = LL_ADC_CHANNEL_10; //2
    adc3_setup[CROSSFM_CV_ADC].sample_time  = LL_ADC_SAMPLINGTIME_480CYCLES;

    adc3_setup[CROSSFM_CV_2_ADC].gpio       = CROSSFM_CV_2_GPIO_Port;
    adc3_setup[CROSSFM_CV_2_ADC].pin        = CROSSFM_CV_2_Pin;
    adc3_setup[CROSSFM_CV_2_ADC].channel    = LL_ADC_CHANNEL_12;
    adc3_setup[CROSSFM_CV_2_ADC].sample_time = LL_ADC_SAMPLINGTIME_480CYCLES;

    adc3_setup[TWIST_CV_ADC].gpio           = TWIST_CV_GPIO_Port;
    adc3_setup[TWIST_CV_ADC].pin            = TWIST_CV_Pin;
    adc3_setup[TWIST_CV_ADC].channel        = LL_ADC_CHANNEL_11; //13
    adc3_setup[TWIST_CV_ADC].sample_time    = LL_ADC_SAMPLINGTIME_480CYCLES;

    adc3_setup[BALANCE_CV_ADC].gpio         = BALANCE_CV_GPIO_Port;
    adc3_setup[BALANCE_CV_ADC].pin          = BALANCE_CV_Pin;
    adc3_setup[BALANCE_CV_ADC].channel      = LL_ADC_CHANNEL_13; //0 
    adc3_setup[BALANCE_CV_ADC].sample_time  = LL_ADC_SAMPLINGTIME_480CYCLES;

    adc3_setup[SCALE_CV_ADC].gpio           = SCALE_CV_GPIO_Port;
    adc3_setup[SCALE_CV_ADC].pin            = SCALE_CV_Pin;
    adc3_setup[SCALE_CV_ADC].channel        = LL_ADC_CHANNEL_0; //1
    adc3_setup[SCALE_CV_ADC].sample_time    = LL_ADC_SAMPLINGTIME_480CYCLES;

}


uint16_t builtin_adc1_raw[ NUM_BUILTIN_ADC1 ];
uint16_t builtin_adc3_raw[ NUM_BUILTIN_ADC3 ];

void adc_init_all(void)
{
    //populate the builtinAdcSetup arrays
    builtin_adc_setup();

    //Initialize and start the ADC and DMA
    init_adc(0, builtin_adc1_raw, NUM_BUILTIN_ADC1, adc1_setup);
    init_adc(1, builtin_adc3_raw, NUM_BUILTIN_ADC3, adc3_setup);
}


void init_adc(uint32_t adcnum, uint16_t *adc_buffer, uint32_t num_channels, builtinAdcSetup *adc_setup)
{
    uint8_t i;
    GPIO_InitTypeDef gpio;
    ADC_TypeDef *adc;

    // LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
    if (adcnum==0) {
        adc = ADC1;
        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_ADC1EN);
    } else{
        adc = ADC3;
        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_ADC3EN);
    }

    // NVIC_SetPriority(ADC_IRQn, 0);
    // NVIC_EnableIRQ(ADC_IRQn);

    // if(__LL_ADC_IS_ENABLED_ALL_COMMON_INSTANCE() == 0) {
    LL_ADC_SetCommonClock(__LL_ADC_COMMON_INSTANCE(adc), LL_ADC_CLOCK_SYNC_PCLK_DIV2);
    // LL_ADC_SetMultiDMATransfer(__LL_ADC_COMMON_INSTANCE(adc), LL_ADC_MULTI_REG_DMA_EACH_ADC);
    // LL_ADC_SetMultiTwoSamplingDelay(__LL_ADC_COMMON_INSTANCE(adc), LL_ADC_MULTI_TWOSMP_DELAY_1CYCLE);
    //} //__LL_ADC_IS_ENABLED_ALL_COMMON_INSTANCE

    // if (LL_ADC_IsEnabled(adc) == 0) {
    LL_ADC_SetResolution(adc, LL_ADC_RESOLUTION_12B);
    LL_ADC_SetResolution(adc, LL_ADC_DATA_ALIGN_RIGHT);
    LL_ADC_SetSequencersScanMode(adc, LL_ADC_SEQ_SCAN_ENABLE);
    LL_ADC_REG_SetTriggerSource(adc, LL_ADC_REG_TRIG_SOFTWARE);
    LL_ADC_REG_SetContinuousMode(adc, LL_ADC_REG_CONV_CONTINUOUS);
    LL_ADC_REG_SetDMATransfer(adc, LL_ADC_REG_DMA_TRANSFER_UNLIMITED);
    LL_ADC_REG_SetFlagEndOfConversion(adc, LL_ADC_REG_FLAG_EOC_SEQUENCE_CONV);
    LL_ADC_REG_SetSequencerLength(adc, num_channels-1);

    for (i=0; i<num_channels; i++)
    {
        gpio.Pin = adc_setup[i].pin;
        gpio.Mode = GPIO_MODE_ANALOG;
        gpio.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(adc_setup[i].gpio, &gpio);
    //  LL_GPIO_SetPinMode(adc_setup[i].gpio, adc_setup[i].pin, LL_GPIO_MODE_ANALOG);
        LL_ADC_REG_SetSequencerRanks(adc, LL_ADC_REG_RANK_1 + i, adc_setup[i].channel);
        LL_ADC_SetChannelSamplingTime(adc, adc_setup[i].channel, adc_setup[i].sample_time);
    }

    LL_ADC_Enable(adc);
    LL_ADC_REG_StartConversionSWStart(adc);

    // }

/*
    ADC_ChannelConfTypeDef  sConfig;
    GPIO_InitTypeDef        gpio;
    uint8_t i;

    //Enable DMA2 clock
    __HAL_RCC_DMA2_CLK_ENABLE();


    hadc[0].Instance = ADC1;
    hadc[1].Instance = ADC3;

    //Initialize ADC1 peripheral
    hadc[adcnum].Init.ClockPrescaler            = ADC_CLOCK_SYNC_PCLK_DIV8;
    hadc[adcnum].Init.Resolution                = ADC_RESOLUTION_12B;
    hadc[adcnum].Init.ScanConvMode              = ENABLE;
    hadc[adcnum].Init.ContinuousConvMode        = ENABLE;
    hadc[adcnum].Init.DiscontinuousConvMode     = DISABLE;
    hadc[adcnum].Init.ExternalTrigConvEdge      = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc[adcnum].Init.ExternalTrigConv          = ADC_EXTERNALTRIGCONV_T1_CC1;//ADC_SOFTWARE_START;
    hadc[adcnum].Init.DataAlign                 = ADC_DATAALIGN_RIGHT;
    hadc[adcnum].Init.NbrOfConversion           = num_channels;
    hadc[adcnum].Init.DMAContinuousRequests     = ENABLE;//DISABLE;
    hadc[adcnum].Init.EOCSelection              = ADC_EOC_SEQ_CONV;//ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc[adcnum]);

    for (i=0; i<num_channels; i++)
    {
        sConfig.Channel         = adc_setup[i].channel;
        sConfig.Rank            = ADC_REGULAR_RANK_1 + i;
        sConfig.SamplingTime    = adc_setup[i].sample_time;
        HAL_ADC_ConfigChannel(&hadc[adcnum], &sConfig);
    }

    HAL_ADC_Start(&hadc[adcnum]);
    HAL_ADC_Start_DMA(&hadc[adcnum], (uint32_t *)adc_buffer, num_channels);
*/
}

//
//This is called from HAL_ADC_Init()
//
/*
void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{
    uint8_t adcnum;

    if(adcHandle->Instance==ADC1)
    {
        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_ADC1EN);
        hdma_adc[0].Instance = DMA2_Stream4;
        hdma_adc[0].Init.Channel = DMA_CHANNEL_0;
        adcnum = 0;
    }
    if(adcHandle->Instance==ADC3)
    {
        SET_BIT(RCC->APB2ENR, RCC_APB2ENR_ADC3EN);
        hdma_adc[1].Instance = DMA2_Stream0;
        hdma_adc[1].Init.Channel = DMA_CHANNEL_2;
        adcnum = 1;
    }

    hdma_adc[adcnum].Init.Direction             = DMA_PERIPH_TO_MEMORY;
    hdma_adc[adcnum].Init.PeriphInc             = DMA_PINC_DISABLE;
    hdma_adc[adcnum].Init.MemInc                = DMA_MINC_ENABLE;
    hdma_adc[adcnum].Init.PeriphDataAlignment   = DMA_PDATAALIGN_HALFWORD;
    hdma_adc[adcnum].Init.MemDataAlignment  = DMA_MDATAALIGN_HALFWORD;
    hdma_adc[adcnum].Init.Mode              = DMA_CIRCULAR;
    hdma_adc[adcnum].Init.Priority          = DMA_PRIORITY_MEDIUM;
    hdma_adc[adcnum].Init.FIFOMode          = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_adc[adcnum]);
    __HAL_LINKDMA(adcHandle,DMA_Handle,hdma_adc[adcnum]);

}
*/
