
#include "adc.hh"
#include "gpio_pins.h"

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

typedef struct builtinAdcSetup{
	GPIO_TypeDef	*gpio;
	uint16_t		pin;
	uint8_t			channel;
	uint8_t			sample_time; //must be a valid ADC_SAMPLETIME_XXXCYCLES
} builtinAdcSetup;

ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc3;
DMA_HandleTypeDef hdma_adc1;
DMA_HandleTypeDef hdma_adc3;

void ADC1_Init(uint16_t *adc_buffer, uint32_t num_channels)
{
  builtinAdcSetup	adc_setup[NUM_BUILTIN_ADC1];

  adc_setup[WARP_POT_ADC].gpio = WARP_POT_GPIO_Port;
  adc_setup[WARP_POT_ADC].pin = WARP_POT_Pin;
  adc_setup[WARP_POT_ADC].channel = ADC_CHANNEL_3;
  adc_setup[WARP_POT_ADC].sample_time = ADC_SAMPLETIME_480CYCLES;

  adc_setup[DETUNE_POT_ADC].gpio = DETUNE_POT_GPIO_Port;
  adc_setup[DETUNE_POT_ADC].pin = DETUNE_POT_Pin;
  adc_setup[DETUNE_POT_ADC].channel = ADC_CHANNEL_4;
  adc_setup[DETUNE_POT_ADC].sample_time = ADC_SAMPLETIME_480CYCLES;

  adc_setup[MOD_POT_ADC].gpio = MOD_POT_GPIO_Port;
  adc_setup[MOD_POT_ADC].pin = MOD_POT_Pin;
  adc_setup[MOD_POT_ADC].channel = ADC_CHANNEL_5;
  adc_setup[MOD_POT_ADC].sample_time = ADC_SAMPLETIME_480CYCLES;

  adc_setup[ROOT_POT_ADC].gpio = ROOT_POT_GPIO_Port;
  adc_setup[ROOT_POT_ADC].pin = ROOT_POT_Pin;
  adc_setup[ROOT_POT_ADC].channel = ADC_CHANNEL_6;
  adc_setup[ROOT_POT_ADC].sample_time = ADC_SAMPLETIME_480CYCLES;

  adc_setup[GRID_POT_ADC].gpio = GRID_POT_GPIO_Port;
  adc_setup[GRID_POT_ADC].pin = GRID_POT_Pin;
  adc_setup[GRID_POT_ADC].channel = ADC_CHANNEL_7;
  adc_setup[GRID_POT_ADC].sample_time = ADC_SAMPLETIME_480CYCLES;

  adc_setup[PITCH_POT_ADC].gpio = PITCH_POT_GPIO_Port;
  adc_setup[PITCH_POT_ADC].pin = PITCH_POT_Pin;
  adc_setup[PITCH_POT_ADC].channel = ADC_CHANNEL_14;
  adc_setup[PITCH_POT_ADC].sample_time = ADC_SAMPLETIME_480CYCLES;

  adc_setup[SPREAD_POT_ADC].gpio = SPREAD_POT_GPIO_Port;
  adc_setup[SPREAD_POT_ADC].pin = SPREAD_POT_Pin;
  adc_setup[SPREAD_POT_ADC].channel = ADC_CHANNEL_15;
  adc_setup[SPREAD_POT_ADC].sample_time = ADC_SAMPLETIME_480CYCLES;

  adc_setup[TILT_POT_ADC].gpio = TILT_POT_GPIO_Port;
  adc_setup[TILT_POT_ADC].pin = TILT_POT_Pin;
  adc_setup[TILT_POT_ADC].channel = ADC_CHANNEL_8;
  adc_setup[TILT_POT_ADC].sample_time = ADC_SAMPLETIME_480CYCLES;

  adc_setup[TWIST_POT_ADC].gpio = TWIST_POT_GPIO_Port;
  adc_setup[TWIST_POT_ADC].pin = TWIST_POT_Pin;
  adc_setup[TWIST_POT_ADC].channel = ADC_CHANNEL_9;
  adc_setup[TWIST_POT_ADC].sample_time = ADC_SAMPLETIME_480CYCLES;

  ADC_ChannelConfTypeDef sConfig;
  GPIO_InitTypeDef gpio;
	uint8_t i;

	//Enable DMA2 clock
	__HAL_RCC_DMA2_CLK_ENABLE();

	//Set GPIO pins to analog
	for (i=0; i<num_channels; i++)
	{
	    gpio.Pin = adc_setup[i].pin;
	    gpio.Mode = GPIO_MODE_ANALOG;
	    gpio.Pull = GPIO_NOPULL;
	    HAL_GPIO_Init(adc_setup[i].gpio, &gpio);
	}

  __HAL_RCC_ADC1_CLK_ENABLE();

  hdma_adc1.Instance = DMA2_Stream4;
  hdma_adc1.Init.Channel = DMA_CHANNEL_0;
  hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
  hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdma_adc1.Init.Mode = DMA_CIRCULAR;
  hdma_adc1.Init.Priority = DMA_PRIORITY_MEDIUM;
  hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&hdma_adc1) != HAL_OK)
    assert_failed(__FILE__, __LINE__);

  __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);

	//Initialize ADC1 peripheral
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV8;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1;//ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = num_channels;
  hadc1.Init.DMAContinuousRequests = ENABLE;//DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;//ADC_EOC_SINGLE_CONV;
	if (HAL_ADC_Init(&hadc1) != HAL_OK)
    assert_failed(__FILE__, __LINE__);

  for (i=0; i<num_channels; i++) {
		sConfig.Channel 		= adc_setup[i].channel;
		sConfig.Rank 			= ADC_REGULAR_RANK_1 + i;
		sConfig.SamplingTime	= adc_setup[i].sample_time;
		if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
			assert_failed(__FILE__, __LINE__);
	}

	//__HAL_ADC_DISABLE_IT(&hadc1, (ADC_IT_EOC | ADC_IT_OVR));

	HAL_ADC_Start(&hadc1);
	HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buffer, num_channels);
}

void ADC3_Init(uint16_t *adc_buffer, uint32_t num_channels)
{
  builtinAdcSetup	adc_setup[NUM_BUILTIN_ADC3];

  adc_setup[SPREAD_CV_1_ADC].gpio = SPREAD_CV_1_GPIO_Port;
  adc_setup[SPREAD_CV_1_ADC].pin = SPREAD_CV_1_Pin;
  adc_setup[SPREAD_CV_1_ADC].channel = ADC_CHANNEL_10;
	adc_setup[SPREAD_CV_1_ADC].sample_time = ADC_SAMPLETIME_480CYCLES;

  adc_setup[WARP_CV_ADC].gpio = WARP_CV_GPIO_Port;
  adc_setup[WARP_CV_ADC].pin = WARP_CV_Pin;
  adc_setup[WARP_CV_ADC].channel = ADC_CHANNEL_11;
  adc_setup[WARP_CV_ADC].sample_time = ADC_SAMPLETIME_480CYCLES;

  adc_setup[SPREAD_CV_2_ADC].gpio = SPREAD_CV_2_GPIO_Port;
  adc_setup[SPREAD_CV_2_ADC].pin = SPREAD_CV_2_Pin;
  adc_setup[SPREAD_CV_2_ADC].channel = ADC_CHANNEL_12;
	adc_setup[SPREAD_CV_2_ADC].sample_time = ADC_SAMPLETIME_480CYCLES;

  adc_setup[TWIST_CV_ADC].gpio = TWIST_CV_GPIO_Port;
  adc_setup[TWIST_CV_ADC].pin = TWIST_CV_Pin;
  adc_setup[TWIST_CV_ADC].channel = ADC_CHANNEL_13;
  adc_setup[TWIST_CV_ADC].sample_time = ADC_SAMPLETIME_480CYCLES;

  adc_setup[TILT_CV_ADC].gpio = TILT_CV_GPIO_Port;
  adc_setup[TILT_CV_ADC].pin = TILT_CV_Pin;
  adc_setup[TILT_CV_ADC].channel = ADC_CHANNEL_0;
  adc_setup[TILT_CV_ADC].sample_time = ADC_SAMPLETIME_480CYCLES;

  adc_setup[GRID_CV_ADC].gpio = GRID_CV_GPIO_Port;
  adc_setup[GRID_CV_ADC].pin = GRID_CV_Pin;
  adc_setup[GRID_CV_ADC].channel = ADC_CHANNEL_1;
  adc_setup[GRID_CV_ADC].sample_time = ADC_SAMPLETIME_480CYCLES;

  adc_setup[MOD_CV_ADC].gpio = MOD_CV_GPIO_Port;
  adc_setup[MOD_CV_ADC].pin = MOD_CV_Pin;
  adc_setup[MOD_CV_ADC].channel = ADC_CHANNEL_2;
  adc_setup[MOD_CV_ADC].sample_time = ADC_SAMPLETIME_480CYCLES;

  ADC_ChannelConfTypeDef sConfig;
  GPIO_InitTypeDef gpio;
	uint8_t i;

	//Enable DMA2 clock
	__HAL_RCC_DMA2_CLK_ENABLE();

	//Set GPIO pins to analog
  for (i=0; i<num_channels; i++) {
	    gpio.Pin = adc_setup[i].pin;
	    gpio.Mode = GPIO_MODE_ANALOG;
	    gpio.Pull = GPIO_NOPULL;
	    HAL_GPIO_Init(adc_setup[i].gpio, &gpio);
	}

  __HAL_RCC_ADC3_CLK_ENABLE();

  hdma_adc3.Instance = DMA2_Stream0;
  hdma_adc3.Init.Channel = DMA_CHANNEL_2;
  hdma_adc3.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_adc3.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_adc3.Init.MemInc = DMA_MINC_ENABLE;
  hdma_adc3.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_adc3.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdma_adc3.Init.Mode = DMA_CIRCULAR;
  hdma_adc3.Init.Priority = DMA_PRIORITY_MEDIUM;
  hdma_adc3.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&hdma_adc3) != HAL_OK)
    assert_failed(__FILE__, __LINE__);

  __HAL_LINKDMA(&hadc3, DMA_Handle, hdma_adc3);

	//Initialize ADC3 peripheral
  hadc3.Instance = ADC3;
  hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV8;
  hadc3.Init.Resolution = ADC_RESOLUTION_12B;
  hadc3.Init.ScanConvMode = ENABLE;
  hadc3.Init.ContinuousConvMode = ENABLE;
  hadc3.Init.DiscontinuousConvMode = DISABLE;
  hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc3.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1;//ADC_SOFTWARE_START;
  hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc3.Init.NbrOfConversion = num_channels;
  hadc3.Init.DMAContinuousRequests = ENABLE;//DISABLE;
  hadc3.Init.EOCSelection = ADC_EOC_SEQ_CONV;//ADC_EOC_SINGLE_CONV;
	if (HAL_ADC_Init(&hadc3) != HAL_OK)
		assert_failed(__FILE__, __LINE__);

	for (i=0; i<num_channels; i++)
	{
    sConfig.Channel = adc_setup[i].channel;
    sConfig.Rank = ADC_REGULAR_RANK_1 + i;
    sConfig.SamplingTime = adc_setup[i].sample_time;
		if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
			assert_failed(__FILE__, __LINE__);
	}

	//__HAL_ADC_DISABLE_IT(&hadc3, (ADC_IT_EOC | ADC_IT_OVR));

	HAL_ADC_Start(&hadc3);
	HAL_ADC_Start_DMA(&hadc3, (uint32_t *)adc_buffer, num_channels);
}

uint16_t adc1_raw[NUM_BUILTIN_ADC1];
uint16_t adc3_raw[NUM_BUILTIN_ADC3];

void adc_init_all(void)
{
	//Initialize and start the ADC and DMA
  ADC1_Init(adc1_raw, NUM_BUILTIN_ADC1);
  ADC3_Init(adc3_raw, NUM_BUILTIN_ADC3);
}

uint16_t get_adc1(int i) {
  return adc1_raw[i];
}

uint16_t get_adc3(int i) {
  return adc3_raw[i];
}
