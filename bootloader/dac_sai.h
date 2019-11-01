#pragma once

#include "stm32f7xx_ll_gpio.h"
#include "gpio_pins.h"

#define DACSAI_REG_GPIO               GPIOE
#define DACSAI_REG_DATA_PIN           LL_GPIO_PIN_0
#define DACSAI_REG_LATCH_PIN          LL_GPIO_PIN_1
#define DACSAI_REG_CLK_PIN            LL_GPIO_PIN_3

#define DAC_LATCH_LOW PIN_OFF(DACSAI_REG_GPIO, DACSAI_REG_LATCH_PIN)
#define DAC_LATCH_HIGH PIN_ON(DACSAI_REG_GPIO, DACSAI_REG_LATCH_PIN)
#define DAC_CLK_LOW PIN_OFF(DACSAI_REG_GPIO, DACSAI_REG_CLK_PIN)
#define DAC_CLK_HIGH PIN_ON(DACSAI_REG_GPIO, DACSAI_REG_CLK_PIN)
#define DAC_DATA_LOW PIN_OFF(DACSAI_REG_GPIO, DACSAI_REG_DATA_PIN)
#define DAC_DATA_HIGH PIN_ON(DACSAI_REG_GPIO, DACSAI_REG_DATA_PIN)

//All register default values are 0 unless noted

//ATTEN_REG1/2
#define NO_ATTEN 0xFF //(default)
#define MAX_ATTEN 0x80

//RST_OSMP_MUTE_REG
#define MUT1 (1<<0) //mute channel 1
#define MUT2 (1<<1) //mute channel 2
#define OVER (1<<6) //0 = 64x oversampling, 1 = 128x
#define SRST (1<<7) //soft reset (initialize registers)

//DEEMP_DACEN_REG
#define DAC1DIS (1<<0)  //DAC1 operation disabled
#define DAC2DIS (1<<1) //DAC2 operation disabled
#define DM12 (1<<4) //De-emphasis enabled
#define DMF_44k  (0<<5) //44.1kHz De-emphasis filter (default)
#define DMF_48k  (1<<5) //48kHz De-emphasis filter 
#define DMF_32k  (1<<6) //32kHz De-emphasis filter 

//FILTER_FORMAT_REG
#define FMT_24RJ (0b000)  //24-bit Right-justified
#define FMT_20RJ (0b001)  //20-bit Right-justified
#define FMT_18RJ (0b010)  //18-bit Right-justified
#define FMT_16RJ (0b011)  //16-bit Right-justified
#define FMT_I2S  (0b100)  //16 to 24-bit I2S
#define FMT_24LJ (0b101)  //16 to 24-bit Left-justified (default)
#define FLT   (1<<5)    //1 = Digital Filter Sharp Rolloff, 0 = Slow Rolloff

//ZEROFLAG_PHASE_REG
#define DREV  (1<<0)  //Inverted output phase
#define ZREV  (1<<1)  //Zero flag output phase: 1: low==zero detect, 0: high==zero detect
#define AZRO  (1<<2)  //1 = L/R common zero flag (pin 11), 0 = independent zero flags

enum PCM1753Registers {
	ATTEN_REG1 = 16,
	ATTEN_REG2 = 17,
	RST_OSMP_MUTE_REG = 18,
	DEEMP_DACEN_REG = 19,
	FILTER_FORMAT_REG = 20,
	ZEROFLAG_PHASE_REG = 22
};


#define DACSAI_SAI                      SAI1
#define DACSAI_SAI_GPIO_AF              LL_GPIO_AF_6
#define DACSAI_SAI_RCC_PERIPHCLK        RCC_PERIPHCLK_SAI1
#define DACSAI_SAI_RCC_CLKSOURCE_PLLI2S RCC_SAI1CLKSOURCE_PLLI2S
#define DACSAI_SAI_GPIO                 GPIOE
#define DACSAI_SAI_MCK_PIN              LL_GPIO_PIN_2
#define DACSAI_SAI_WS_PIN               LL_GPIO_PIN_4
#define DACSAI_SAI_SCK_PIN              LL_GPIO_PIN_5
#define DACSAI_SAI_SDO_PIN              LL_GPIO_PIN_6

#define DACSAI_SAI_TX_BLOCK           SAI1_Block_A
#define DACSAI_SAI_TX_DMA             DMA2
#define DACSAI_SAI_TX_DMA_ISR         LISR
#define DACSAI_SAI_TX_DMA_IFCR        LIFCR
#define DACSAI_SAI_TX_DMA_STREAM      LL_DMA_STREAM_1
#define DACSAI_SAI_TX_DMA_IRQn        DMA2_Stream1_IRQn
#define DACSAI_SAI_TX_DMA_IRQHandler  DMA2_Stream1_IRQHandler
#define DACSAI_SAI_TX_DMA_CHANNEL     LL_DMA_CHANNEL_0

#define DACSAI_SAI_TX_DMA_FLAG_TC     DMA_FLAG_TCIF1_5
#define DACSAI_SAI_TX_DMA_FLAG_HT     DMA_FLAG_HTIF1_5
#define DACSAI_SAI_TX_DMA_FLAG_FE     DMA_FLAG_FEIF1_5
#define DACSAI_SAI_TX_DMA_FLAG_TE     DMA_FLAG_TEIF1_5
#define DACSAI_SAI_TX_DMA_FLAG_DME    DMA_FLAG_DMEIF1_5


uint8_t init_dac(void);
