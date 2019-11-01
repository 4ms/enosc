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


//From HAL:
#define SAI_SYNCEXT_DISABLE          0
#define SAI_SYNCEXT_OUTBLOCKA_ENABLE 1
#define SAI_SYNCEXT_OUTBLOCKB_ENABLE 2

#define SAI_I2S_STANDARD      0
#define SAI_I2S_MSBJUSTIFIED  1
#define SAI_I2S_LSBJUSTIFIED  2
#define SAI_PCM_LONG          3
#define SAI_PCM_SHORT         4

#define SAI_PROTOCOL_DATASIZE_16BIT         0
#define SAI_PROTOCOL_DATASIZE_16BITEXTENDED 1
#define SAI_PROTOCOL_DATASIZE_24BIT         2
#define SAI_PROTOCOL_DATASIZE_32BIT         3

#define SAI_MODEMASTER_TX         ((uint32_t)0x00000000U)
#define SAI_MODEMASTER_RX         ((uint32_t)SAI_xCR1_MODE_0)
#define SAI_MODESLAVE_TX          ((uint32_t)SAI_xCR1_MODE_1)
#define SAI_MODESLAVE_RX          ((uint32_t)(SAI_xCR1_MODE_1 | SAI_xCR1_MODE_0))

#define SAI_FREE_PROTOCOL                 ((uint32_t)0x00000000U)
#define SAI_SPDIF_PROTOCOL                ((uint32_t)SAI_xCR1_PRTCFG_0)
#define SAI_AC97_PROTOCOL                 ((uint32_t)SAI_xCR1_PRTCFG_1)

#define SAI_DATASIZE_8     ((uint32_t)SAI_xCR1_DS_1)
#define SAI_DATASIZE_10    ((uint32_t)(SAI_xCR1_DS_1 | SAI_xCR1_DS_0))
#define SAI_DATASIZE_16    ((uint32_t)SAI_xCR1_DS_2)
#define SAI_DATASIZE_20    ((uint32_t)(SAI_xCR1_DS_2 | SAI_xCR1_DS_0))
#define SAI_DATASIZE_24    ((uint32_t)(SAI_xCR1_DS_2 | SAI_xCR1_DS_1))
#define SAI_DATASIZE_32    ((uint32_t)(SAI_xCR1_DS_2 | SAI_xCR1_DS_1 | SAI_xCR1_DS_0))

#define SAI_FIRSTBIT_MSB                  ((uint32_t)0x00000000U)
#define SAI_FIRSTBIT_LSB                  ((uint32_t)SAI_xCR1_LSBFIRST)

#define SAI_CLOCKSTROBING_FALLINGEDGE     0
#define SAI_CLOCKSTROBING_RISINGEDGE      1

#define SAI_ASYNCHRONOUS                  0 /*!< Asynchronous */
#define SAI_SYNCHRONOUS                   1 /*!< Synchronous with other block of same SAI */
#define SAI_SYNCHRONOUS_EXT_SAI1          2 /*!< Synchronous with other SAI, SAI1 */
#define SAI_SYNCHRONOUS_EXT_SAI2          3 /*!< Synchronous with other SAI, SAI2 */

#define SAI_OUTPUTDRIVE_DISABLE          ((uint32_t)0x00000000U)
#define SAI_OUTPUTDRIVE_ENABLE           ((uint32_t)SAI_xCR1_OUTDRIV)

#define SAI_MASTERDIVIDER_ENABLE         ((uint32_t)0x00000000U)
#define SAI_MASTERDIVIDER_DISABLE        ((uint32_t)SAI_xCR1_NODIV)

#define SAI_FS_STARTFRAME                 ((uint32_t)0x00000000U)
#define SAI_FS_CHANNEL_IDENTIFICATION     ((uint32_t)SAI_xFRCR_FSDEF)

#define SAI_FS_ACTIVE_LOW                  ((uint32_t)0x00000000U)
#define SAI_FS_ACTIVE_HIGH                 ((uint32_t)SAI_xFRCR_FSPOL)

#define SAI_FS_FIRSTBIT                   ((uint32_t)0x00000000U)
#define SAI_FS_BEFOREFIRSTBIT             ((uint32_t)SAI_xFRCR_FSOFF)

#define SAI_SLOTSIZE_DATASIZE             ((uint32_t)0x00000000U)
#define SAI_SLOTSIZE_16B                  ((uint32_t)SAI_xSLOTR_SLOTSZ_0)
#define SAI_SLOTSIZE_32B                  ((uint32_t)SAI_xSLOTR_SLOTSZ_1)

#define SAI_SLOT_NOTACTIVE           ((uint32_t)0x00000000U)
#define SAI_SLOTACTIVE_0             ((uint32_t)0x00000001U)
#define SAI_SLOTACTIVE_1             ((uint32_t)0x00000002U)
#define SAI_SLOTACTIVE_2             ((uint32_t)0x00000004U)
#define SAI_SLOTACTIVE_3             ((uint32_t)0x00000008U)
#define SAI_SLOTACTIVE_4             ((uint32_t)0x00000010U)
#define SAI_SLOTACTIVE_5             ((uint32_t)0x00000020U)
#define SAI_SLOTACTIVE_6             ((uint32_t)0x00000040U)
#define SAI_SLOTACTIVE_7             ((uint32_t)0x00000080U)
#define SAI_SLOTACTIVE_8             ((uint32_t)0x00000100U)
#define SAI_SLOTACTIVE_9             ((uint32_t)0x00000200U)
#define SAI_SLOTACTIVE_10            ((uint32_t)0x00000400U)
#define SAI_SLOTACTIVE_11            ((uint32_t)0x00000800U)
#define SAI_SLOTACTIVE_12            ((uint32_t)0x00001000U)
#define SAI_SLOTACTIVE_13            ((uint32_t)0x00002000U)
#define SAI_SLOTACTIVE_14            ((uint32_t)0x00004000U)
#define SAI_SLOTACTIVE_15            ((uint32_t)0x00008000U)
#define SAI_SLOTACTIVE_ALL           ((uint32_t)0x0000FFFFU)

#define SAI_STEREOMODE               ((uint32_t)0x00000000U)
#define SAI_MONOMODE                 ((uint32_t)SAI_xCR1_MONO)

#define SAI_OUTPUT_NOTRELEASED        ((uint32_t)0x00000000U)
#define SAI_OUTPUT_RELEASED           ((uint32_t)SAI_xCR2_TRIS)

#define SAI_FIFOTHRESHOLD_EMPTY  ((uint32_t)0x00000000U)
#define SAI_FIFOTHRESHOLD_1QF    ((uint32_t)(SAI_xCR2_FTH_0))
#define SAI_FIFOTHRESHOLD_HF     ((uint32_t)(SAI_xCR2_FTH_1))
#define SAI_FIFOTHRESHOLD_3QF    ((uint32_t)(SAI_xCR2_FTH_1 | SAI_xCR2_FTH_0))
#define SAI_FIFOTHRESHOLD_FULL   ((uint32_t)(SAI_xCR2_FTH_2))

#define SAI_NOCOMPANDING                 ((uint32_t)0x00000000U)
#define SAI_ULAW_1CPL_COMPANDING         ((uint32_t)(SAI_xCR2_COMP_1))
#define SAI_ALAW_1CPL_COMPANDING         ((uint32_t)(SAI_xCR2_COMP_1 | SAI_xCR2_COMP_0))
#define SAI_ULAW_2CPL_COMPANDING         ((uint32_t)(SAI_xCR2_COMP_1 | SAI_xCR2_CPL))
#define SAI_ALAW_2CPL_COMPANDING         ((uint32_t)(SAI_xCR2_COMP_1 | SAI_xCR2_COMP_0 | SAI_xCR2_CPL))

#define SAI_ZERO_VALUE                   ((uint32_t)0x00000000U)
#define SAI_LAST_SENT_VALUE              ((uint32_t)SAI_xCR2_MUTEVAL)

uint8_t init_dac(void);
