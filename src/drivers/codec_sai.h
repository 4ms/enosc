/*
 * codec_sai.h
 */

#pragma once

#include <stm32f7xx.h>

//
// Codec buffer size
// 

#define codec_BUFF_LEN 		1024						/* DMA rx/tx buffer size, in number of DMA Periph/MemAlign-sized elements (words) */
#define codec_HT_LEN 		(codec_BUFF_LEN>>1) 		/* Half Transfer buffer size (both channels interleved)*/
#define codec_HT_CHAN_LEN 	(codec_HT_LEN>>1) 			/* Half Transfer buffer size per channel */


//
// Codec SAI pins
//

#define CODEC_SAI						SAI1

#define CODEC_SAI_GPIO_AF				GPIO_AF6_SAI1
#define CODEC_SAI_GPIO_CLOCK_ENABLE		__HAL_RCC_GPIOE_CLK_ENABLE
#define	CODEC_SAI_RCC_PERIPHCLK			RCC_PERIPHCLK_SAI1
#define	CODEC_SAI_RCC_CLKSOURCE_PLLI2S	RCC_SAI1CLKSOURCE_PLLI2S

#define CODEC_SAI_MCK_GPIO				GPIOE
#define CODEC_SAI_MCK_PIN				GPIO_PIN_2

#define CODEC_SAI_GPIO_WS				GPIOE
#define CODEC_SAI_WS_PIN				GPIO_PIN_4

#define CODEC_SAI_GPIO_SCK				GPIOE
#define CODEC_SAI_SCK_PIN				GPIO_PIN_5

#define CODEC_SAI_GPIO_SDO				GPIOE
#define CODEC_SAI_SDO_PIN				GPIO_PIN_6 		// SAI1_SD_A (MTX)

#define CODEC_SAI_GPIO_SDI				GPIOE
#define CODEC_SAI_SDI_PIN				GPIO_PIN_3		// SAI1_SD_B (MRX) 

#define CODEC_SaixClockSelection		Sai1ClockSelection
#define CODEC_SAI_CLOCK_DISABLE			__HAL_RCC_SAI1_CLK_DISABLE
#define CODEC_SAI_CLOCK_ENABLE			__HAL_RCC_SAI1_CLK_ENABLE

#define CODEC_SAI_DMA_CLOCK_ENABLE		__HAL_RCC_DMA2_CLK_ENABLE
#define CODEC_SAI_DMA_CLOCK_DISABLE		__HAL_RCC_DMA2_CLK_DISABLE

//SAI1 A: Master TX (MTX == Master SDO == Slave SDI == DAC)
#define CODEC_SAI_TX_BLOCK				SAI1_Block_A
#define	CODEC_SAI_TX_DMA				DMA2
#define	CODEC_SAI_TX_DMA_ISR			LISR
#define	CODEC_SAI_TX_DMA_IFCR			LIFCR
#define CODEC_SAI_TX_DMA_STREAM			DMA2_Stream1
#define CODEC_SAI_TX_DMA_IRQn 			DMA2_Stream1_IRQn
#define CODEC_SAI_TX_DMA_IRQHandler		DMA2_Stream1_IRQHandler
#define CODEC_SAI_TX_DMA_CHANNEL		DMA_CHANNEL_0

#define CODEC_SAI_TX_DMA_FLAG_TC		DMA_FLAG_TCIF1_5
#define CODEC_SAI_TX_DMA_FLAG_HT		DMA_FLAG_HTIF1_5
#define CODEC_SAI_TX_DMA_FLAG_FE		DMA_FLAG_FEIF1_5
#define CODEC_SAI_TX_DMA_FLAG_TE		DMA_FLAG_TEIF1_5
#define CODEC_SAI_TX_DMA_FLAG_DME		DMA_FLAG_DMEIF1_5

//SAI1 B: Master RX (MRX == Master SDI == Slave SDO == ADC)
#define CODEC_SAI_RX_BLOCK				SAI1_Block_B
#define	CODEC_SAI_RX_DMA				DMA2
#define	CODEC_SAI_RX_DMA_ISR			HISR
#define	CODEC_SAI_RX_DMA_IFCR			HIFCR
#define CODEC_SAI_RX_DMA_STREAM			DMA2_Stream5
#define CODEC_SAI_RX_DMA_IRQn			DMA2_Stream5_IRQn
#define CODEC_SAI_RX_DMA_IRQHandler		DMA2_Stream5_IRQHandler
#define CODEC_SAI_RX_DMA_CHANNEL		DMA_CHANNEL_0

#define CODEC_SAI_RX_DMA_FLAG_TC		DMA_FLAG_TCIF1_5
#define CODEC_SAI_RX_DMA_FLAG_HT		DMA_FLAG_HTIF1_5
#define CODEC_SAI_RX_DMA_FLAG_FE		DMA_FLAG_FEIF1_5
#define CODEC_SAI_RX_DMA_FLAG_TE		DMA_FLAG_TEIF1_5
#define CODEC_SAI_RX_DMA_FLAG_DME		DMA_FLAG_DMEIF1_5



void init_SAI_clock(uint32_t sample_rate);
void codec_SAI_init(uint32_t sample_rate);

void Init_SAIDMA(void);
void DeInit_I2S_Clock(void);
void DeInit_SAIDMA(void);
void start_audio(void);
void init_audio_DMA(void);

void reboot_codec(uint32_t sample_rate);