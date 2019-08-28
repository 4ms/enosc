#pragma once

#include "buffer.hh"
#include "parameters.hh"
#include "hal.hh"

//
// Codec SAI pins
//

#define DACSAI_SAI                      SAI1

#define DACSAI_SAI_GPIO_AF              GPIO_AF6_SAI1
#define DACSAI_SAI_GPIO_CLOCK_ENABLE    __HAL_RCC_GPIOE_CLK_ENABLE
#define DACSAI_SAI_RCC_PERIPHCLK        RCC_PERIPHCLK_SAI1
#define DACSAI_SAI_RCC_CLKSOURCE_PLLI2S RCC_SAI1CLKSOURCE_PLLI2S

#define DACSAI_SAI_MCK_GPIO             GPIOE
#define DACSAI_SAI_MCK_PIN              GPIO_PIN_2

#define DACSAI_SAI_GPIO_WS              GPIOE
#define DACSAI_SAI_WS_PIN               GPIO_PIN_4

#define DACSAI_SAI_GPIO_SCK             GPIOE
#define DACSAI_SAI_SCK_PIN              GPIO_PIN_5

#define DACSAI_SAI_GPIO_SDO            GPIOE
#define DACSAI_SAI_SDO_PIN             GPIO_PIN_6

#define DACSAI_SaixClockSelection     Sai1ClockSelection
#define DACSAI_SAI_CLOCK_DISABLE      __HAL_RCC_SAI1_CLK_DISABLE
#define DACSAI_SAI_CLOCK_ENABLE       __HAL_RCC_SAI1_CLK_ENABLE

#define DACSAI_SAI_DMA_CLOCK_ENABLE   __HAL_RCC_DMA2_CLK_ENABLE
#define DACSAI_SAI_DMA_CLOCK_DISABLE  __HAL_RCC_DMA2_CLK_DISABLE

//SAI1 A:
#define DACSAI_SAI_TX_BLOCK           SAI1_Block_A
#define DACSAI_SAI_TX_DMA             DMA2
#define DACSAI_SAI_TX_DMA_ISR         LISR
#define DACSAI_SAI_TX_DMA_IFCR        LIFCR
#define DACSAI_SAI_TX_DMA_STREAM      DMA2_Stream1
#define DACSAI_SAI_TX_DMA_IRQn        DMA2_Stream1_IRQn
#define DACSAI_SAI_TX_DMA_IRQHandler  DMA2_Stream1_IRQHandler
#define DACSAI_SAI_TX_DMA_CHANNEL     DMA_CHANNEL_0

#define DACSAI_SAI_TX_DMA_FLAG_TC     DMA_FLAG_TCIF1_5
#define DACSAI_SAI_TX_DMA_FLAG_HT     DMA_FLAG_HTIF1_5
#define DACSAI_SAI_TX_DMA_FLAG_FE     DMA_FLAG_FEIF1_5
#define DACSAI_SAI_TX_DMA_FLAG_TE     DMA_FLAG_TEIF1_5
#define DACSAI_SAI_TX_DMA_FLAG_DME    DMA_FLAG_DMEIF1_5


//
// Codec BitBang register setup pins
// 
#define DACSAI_REG_GPIO_CLOCK_ENABLE  __HAL_RCC_GPIOE_CLK_ENABLE
#define DACSAI_REG_GPIO               GPIOE
#define DACSAI_REG_DATA_PIN           GPIO_PIN_0
#define DACSAI_REG_LATCH_PIN          GPIO_PIN_1
#define DACSAI_REG_CLK_PIN            GPIO_PIN_3


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



void register_codec_isr(void f());

template<int sample_rate, int block_size, class T>
struct Codec : Nocopy {

  Codec() {
    instance_ = this;
    register_codec_isr(handler__IN_ITCM_);

    // Setup PLL clock for codec
    init_SAI_clock();

    gpio_.Init();

    //Start Codec SAI
    SAI_init();
    init_audio_DMA();

    bb_regsetup_.Init();
  }

  void Start() {
    HAL_NVIC_EnableIRQ(DACSAI_SAI_TX_DMA_IRQn);
  }

private:

  DMA_HandleTypeDef hdma_tx;

  SAI_HandleTypeDef hsai_tx;

  union {
    struct {
      Buffer<Frame, block_size> tx[2];
    } buffers;
    struct {
      uint8_t tx[block_size * sizeof(Frame)][2];
    } arrays;
  };

  static Codec *instance_;

  static void handler__IN_ITCM_() { 
    instance_->ISR__IN_ITCM_();
  }

  void ISR__IN_ITCM_() {
    //Read the interrupt status register (ISR)
    uint32_t tmpisr = DACSAI_SAI_TX_DMA->DACSAI_SAI_TX_DMA_ISR;

    if ((tmpisr & __HAL_DMA_GET_TC_FLAG_INDEX(&instance_->hdma_tx)) 
        && __HAL_DMA_GET_IT_SOURCE(&instance_->hdma_tx, DMA_IT_TC)) {
      // Transfer Complete (TC) -> Point to 2nd half of buffers
      __HAL_DMA_CLEAR_FLAG(&instance_->hdma_tx,
                           __HAL_DMA_GET_TC_FLAG_INDEX(&instance_->hdma_tx));
      static_cast<T&>(*this).template CodecCallback<block_size>(instance_->buffers.tx[1]);

    } else if ((tmpisr & __HAL_DMA_GET_HT_FLAG_INDEX(&instance_->hdma_tx))
               && __HAL_DMA_GET_IT_SOURCE(&instance_->hdma_tx, DMA_IT_HT)) {
      // Half Transfer complete (HT) -> Point to 1st half of buffers
      __HAL_DMA_CLEAR_FLAG(&instance_->hdma_tx,
                           __HAL_DMA_GET_HT_FLAG_INDEX(&instance_->hdma_tx));
      static_cast<T&>(*this).template CodecCallback<block_size>(instance_->buffers.tx[0]);
    }
  }

  struct GPIO {
    // TODO init with constructor
    void Init() {
      GPIO_InitTypeDef gpio;

      //BitBang register setup pins:
      DACSAI_REG_GPIO_CLOCK_ENABLE();

      gpio.Mode     = GPIO_MODE_OUTPUT_PP;
      gpio.Pull     = GPIO_NOPULL;
      gpio.Speed    = GPIO_SPEED_FREQ_MEDIUM;
      gpio.Pin      = DACSAI_REG_DATA_PIN | DACSAI_REG_LATCH_PIN | DACSAI_REG_CLK_PIN;
      HAL_GPIO_Init(DACSAI_REG_GPIO, &gpio);

      //bb_regsetup_.latch_pin(LOW);
      HAL_GPIO_WritePin(DACSAI_REG_GPIO, DACSAI_REG_LATCH_PIN, GPIO_PIN_RESET);

      DACSAI_SAI_GPIO_CLOCK_ENABLE();

      // SAI pins:
      gpio.Mode = GPIO_MODE_AF_PP;
      gpio.Pull = GPIO_NOPULL;
      gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
      gpio.Alternate  = DACSAI_SAI_GPIO_AF;

      gpio.Pin = DACSAI_SAI_WS_PIN; HAL_GPIO_Init(DACSAI_SAI_GPIO_WS, &gpio);
      gpio.Pin = DACSAI_SAI_SCK_PIN;  HAL_GPIO_Init(DACSAI_SAI_GPIO_SCK, &gpio);
      gpio.Pin = DACSAI_SAI_SDO_PIN;  HAL_GPIO_Init(DACSAI_SAI_GPIO_SDO, &gpio);
      gpio.Pin = DACSAI_SAI_MCK_PIN;  HAL_GPIO_Init(DACSAI_SAI_MCK_GPIO, &gpio);
    }

  } gpio_;

  struct BBRegSetup {
    void Init() {
      Write(ATTEN_REG1, NO_ATTEN);
      Write(ATTEN_REG2, NO_ATTEN);
      Write(RST_OSMP_MUTE_REG, OVER);
      Write(DEEMP_DACEN_REG, 0);
      Write(FILTER_FORMAT_REG, FMT_I2S);
      Write(ZEROFLAG_PHASE_REG, DREV);
    }

    void DeInit() {
      HAL_GPIO_DeInit(DACSAI_REG_GPIO, DACSAI_REG_DATA_PIN | DACSAI_REG_LATCH_PIN | DACSAI_REG_CLK_PIN);
    }

    void Reset() {
      Write(RST_OSMP_MUTE_REG, SRST); //Reset all registers
    }

  private:
    void Write(enum PCM1753Registers reg_addr, uint8_t reg_value) {

      latch_pin(HIGH);
      HAL_Delay(1);

      latch_pin(LOW);
      clk_pin(LOW);
      HAL_Delay(1);

      uint16_t data = (reg_addr << 8) | reg_value;

      for (int i=16; i--;) {
        if (data & (1<<i))
          data_pin(HIGH);
        else
          data_pin(LOW);

        HAL_Delay(1);

        clk_pin(HIGH);
        HAL_Delay(1);

        clk_pin(LOW);
        HAL_Delay(1);
      }

      latch_pin(HIGH);
      HAL_Delay(1);
    }

    enum PinStates { LOW = 0, HIGH = 1 };

    void data_pin(enum PinStates state) {
      HAL_GPIO_WritePin(DACSAI_REG_GPIO, DACSAI_REG_DATA_PIN, (GPIO_PinState)state);
    }

    void clk_pin(enum PinStates state) {
      HAL_GPIO_WritePin(DACSAI_REG_GPIO, DACSAI_REG_CLK_PIN, (GPIO_PinState)state);
    }

    void latch_pin(enum PinStates state) {
      HAL_GPIO_WritePin(DACSAI_REG_GPIO, DACSAI_REG_LATCH_PIN, (GPIO_PinState)state);
    }

  } bb_regsetup_;

  void Reboot() {
    //Take everything down...
    bb_regsetup_.Reset();
    HAL_Delay(1);

    DeInit_I2S_Clock();
    DeInit_SAIDMA();

    //...and bring it all back up
    this.init_SAI_clock(sample_rate);

    gpio_.Init();
    SAI_init(sample_rate);
    init_audio_DMA();

    bb_regsetup_.Init(sample_rate);

    Start();
  }

  void init_SAI_clock() {
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;
    PeriphClkInitStruct.PeriphClockSelection = DACSAI_SAI_RCC_PERIPHCLK;

    //PLL input = HSE / PLLM = 16000000 / 16 = 1000000
    //PLLI2S = 1000000 * PLLI2SN / PLLI2SQ / PLLI2SDivQ

    if (sample_rate==44100)
    {
      //44.1kHz * 256 == 11 289 600
      // 		1000000 * 384 / 2 / 17
      //		= 11 294 117 = +0.04%

      PeriphClkInitStruct.PLLI2S.PLLI2SN 	= 384;	// mult by 384 = 384MHz
      PeriphClkInitStruct.PLLI2S.PLLI2SQ 	= 2;  	// div by 2 = 192MHz
      PeriphClkInitStruct.PLLI2SDivQ 		= 17; 	// div by 17 = 11.294117MHz
      // div by 256 for bit rate = 44.117kHz
    }

    else if (sample_rate==48000)
    {
      //48kHz * 256 == 12.288 MHz
      //		1000000 * 344 / 4 / 7
      //		= 12.285714MHz = -0.01%

      PeriphClkInitStruct.PLLI2S.PLLI2SN 	= 344;	// mult by 344 = 344MHz
      PeriphClkInitStruct.PLLI2S.PLLI2SQ 	= 4;  	// div by 4 = 86MHz
      PeriphClkInitStruct.PLLI2SDivQ 		= 7; 	// div by 7 = 12.285714MHz
      // div by 256 for bit rate = 47.991kHz
    }

    else if (sample_rate==96000)
    {
      //96kHz * 256 == 24.576 MHz
      //		1000000 * 344 / 2 / 7
      //		= 24.571429MHz = -0.02%
		
      PeriphClkInitStruct.PLLI2S.PLLI2SN 	= 344;	// mult by 344 = 344MHz
      PeriphClkInitStruct.PLLI2S.PLLI2SQ 	= 2;  	// div by 2 = 172MHz
      PeriphClkInitStruct.PLLI2SDivQ 		= 7; 	// div by 7 = 24.571429MHz
      // div by 256 for bit rate = 95.982kHz
    }
    else 
      return; //exit if sample_rate is not valid

    PeriphClkInitStruct.DACSAI_SaixClockSelection 		= DACSAI_SAI_RCC_CLKSOURCE_PLLI2S;
    hal_assert(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct));
  }

  void SAI_init() {
    DACSAI_SAI_CLOCK_ENABLE();

    if (!IS_SAI_AUDIO_FREQUENCY(sample_rate)) return;

    hsai_tx.Instance 				    = DACSAI_SAI_TX_BLOCK;
    hsai_tx.Init.AudioMode 		  = SAI_MODEMASTER_TX;
    hsai_tx.Init.Synchro 			  = SAI_ASYNCHRONOUS;
    hsai_tx.Init.OutputDrive 		= SAI_OUTPUTDRIVE_DISABLE;
    hsai_tx.Init.NoDivider 		  = SAI_MASTERDIVIDER_ENABLE;
    hsai_tx.Init.FIFOThreshold 	= SAI_FIFOTHRESHOLD_EMPTY;
    hsai_tx.Init.AudioFrequency	= sample_rate;
    hsai_tx.Init.SynchroExt 		= SAI_SYNCEXT_DISABLE;
    hsai_tx.Init.MonoStereoMode = SAI_STEREOMODE;
    hsai_tx.Init.CompandingMode = SAI_NOCOMPANDING;
    hsai_tx.Init.TriState 		  = SAI_OUTPUT_NOTRELEASED;

    // Don't initialize yet, we have to de-init the DMA first
    HAL_SAI_DeInit(&hsai_tx);
  }

  void DeInit_I2S_Clock() {
    HAL_RCCEx_DisablePLLI2S();
  }

  void DeInit_SAIDMA() {
    HAL_NVIC_DisableIRQ(DACSAI_SAI_TX_DMA_IRQn); 

    HAL_RCCEx_DisablePLLSAI();

    DACSAI_SAI_CLOCK_DISABLE();

    HAL_SAI_DeInit(&hsai_tx);
    HAL_DMA_Abort(&hdma_tx);
    HAL_DMA_DeInit(&hdma_tx);
  }

  void init_audio_DMA() {

    // Prepare the DMA for TX (but don't enable yet)
    DACSAI_SAI_DMA_CLOCK_ENABLE();

    hdma_tx.Instance                  = DACSAI_SAI_TX_DMA_STREAM;
    hdma_tx.Init.Channel              = DACSAI_SAI_TX_DMA_CHANNEL;
    hdma_tx.Init.Direction            = DMA_MEMORY_TO_PERIPH;
    hdma_tx.Init.PeriphInc            = DMA_PINC_DISABLE;
    hdma_tx.Init.MemInc               = DMA_MINC_ENABLE;
    hdma_tx.Init.PeriphDataAlignment  = DMA_PDATAALIGN_HALFWORD;
    hdma_tx.Init.MemDataAlignment     = DMA_PDATAALIGN_HALFWORD;
    hdma_tx.Init.Mode                 = DMA_CIRCULAR;
    hdma_tx.Init.Priority             = DMA_PRIORITY_HIGH;
    hdma_tx.Init.FIFOMode             = DMA_FIFOMODE_DISABLE;
    hdma_tx.Init.MemBurst             = DMA_MBURST_SINGLE;
    hdma_tx.Init.PeriphBurst          = DMA_PBURST_SINGLE;

    HAL_DMA_DeInit(&hdma_tx);

    // Must initialize the SAI before initializing the DMA
    hal_assert(HAL_SAI_InitProtocol(&hsai_tx, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_16BITEXTENDED, 2));

    hal_assert(HAL_DMA_Init(&hdma_tx));
    __HAL_LINKDMA(&hsai_tx, hdmatx, hdma_tx);

    // DMA IRQ and start DMA
    HAL_NVIC_SetPriority(DACSAI_SAI_TX_DMA_IRQn, 0, 0);
    HAL_NVIC_DisableIRQ(DACSAI_SAI_TX_DMA_IRQn); 
    HAL_SAI_Transmit_DMA(&hsai_tx, arrays.tx[0], block_size * 2 * 2);
  }

};

template<int sample_rate, int block_size, class T>
Codec<sample_rate, block_size, T> *Codec<sample_rate, block_size, T>::instance_;
