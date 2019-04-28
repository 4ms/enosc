#pragma once

#include "buffer.hh"
#include "parameters.hh"
#include "hal.hh"

//
// I2C Config
//
#define CODEC_I2C						I2C1
#define CODEC_I2C_CLK_ENABLE			__HAL_RCC_I2C1_CLK_ENABLE
#define CODEC_I2C_CLK_DISABLE			__HAL_RCC_I2C1_CLK_DISABLE
#define CODEC_I2C_GPIO_CLOCK_ENABLE		__HAL_RCC_GPIOB_CLK_ENABLE
#define CODEC_I2C_GPIO_AF				GPIO_AF4_I2C1
#define CODEC_I2C_GPIO					GPIOB
#define CODEC_I2C_SCL_PIN				GPIO_PIN_8
#define CODEC_I2C_SDA_PIN				GPIO_PIN_7

#define W8731_ADDR_0 0x1A
#define W8731_ADDR_1 0x1B
#define W8731_NUM_REGS 10

#define CODEC_ADDRESS           (W8731_ADDR_0<<1)

#define MCLK_SRC_STM 0
#define MCLK_SRC_EXTERNAL 1

#define CODEC_MCLK_SRC 			MCLK_SRC_STM

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


//Registers:
#define WM8731_REG_INBOTH 		8
#define WM8731_REG_INMUTE 		7
#define WM8731_REG_POWERDOWN 	6
#define WM8731_REG_INVOL 		0

#define WM8731_REG_SAMPLE_CTRL	0x08
#define WM8731_REG_RESET 		0x0F

#define VOL_p12dB	0b11111 /*+12dB*/
#define VOL_0dB		0b10111 /*0dB*/
#define VOL_n1dB	0b10110 /*-1.5dB*/
#define VOL_n3dB	0b10101 /*-3dB*/
#define VOL_n6dB	0b10011 /*-6dB*/
#define VOL_n12dB	15 		/*-12dB*/
#define VOL_n15dB	13 		/*-15dB*/
/*1.5dB steps down to..*/
#define VOL_n34dB	0b00000 /*-34.5dB*/

//Register 4: Analogue Audio Path Control
#define MICBOOST 		(1 << 0)	/* Boost Mic level */
#define MUTEMIC			(1 << 1)	/* Mute Mic to ADC */
#define INSEL_mic		(1 << 2)	/* Mic Select*/
#define INSEL_line		(0 << 2)	/* LineIn Select*/
#define BYPASS_EN			(1 << 3)	/* Bypass Enable */
#define DACSEL			(1 << 4)	/* Select DAC */
#define SIDETONE		(1 << 5)	/* Enable Sidetone */
#define SIDEATT_neg15dB	(0b11 << 6)
#define SIDEATT_neg12dB	(0b10 << 6)
#define SIDEATT_neg9dB	(0b01 << 6)
#define SIDEATT_neg6dB	(0b00 << 6)


//Register 5: Digital Audio Path Control
#define ADCHPFDisable 1 				/* ADC High Pass Filter */
#define ADCHPFEnable 0
#define DEEMPH_48k		(0b11 << 1) 	/* De-emphasis Control */
#define DEEMPH_44k		(0b10 << 1)
#define DEEMPH_32k 		(0b01 << 1)
#define DEEMPH_disable	(0b00 << 1)
#define DACMU_enable	(1 << 3) 		/* DAC Soft Mute Control */
#define DACMU_disable	(0 << 3)
#define HPOR_store		(1 << 4) 		/* Store DC offset when HPF disabled */
#define HPOR_clear		(0 << 4)

//Register 6: Power Down Control: 1= enable power down, 0=disable power down
#define PD_LINEIN		(1<<0)
#define PD_MIC			(1<<1)
#define PD_ADC			(1<<2)
#define PD_DAC			(1<<3)
#define PD_OUT			(1<<4)
#define PD_OSC			(1<<5)
#define PD_CLKOUT		(1<<6)
#define PD_POWEROFF		(1<<7)


//Register 7: Digital Audio Interface Format
#define format_MSB_Right 0
#define format_MSB_Left 1
#define format_I2S 2
#define format_DSP 3
#define format_16b (0<<2)
#define format_20b (1<<2)
#define format_24b (2<<2)
#define format_32b (3<<2)


//Register: Sample Rate Controls
#define	SR_USB_NORM	(1<<0)		//1=USB (250/272fs), 0=Normal Mode (256/384fs)
#define SR_BOSR		(1<<1)		//Base Over-Sampling Rate: 0=250/256fs, 1=272/384fs (also 128/192)
#define SR_NORM_8K 	(0b1011 << 2)
#define SR_NORM_32K (0b0110 << 2)
#define SR_NORM_44K (0b1000 << 2)
#define	SR_NORM_48K	(0b0000 << 2)
#define SR_NORM_88K (0b1111 << 2)
#define SR_NORM_96K (0b0111 << 2)

// Oddness:
// format_I2S does not work with I2S2 on the STM32F427Z (works on the 427V) in Master TX mode (I2S2ext is RX)
// The RX data is shifted left 2 bits (x4) as it comes in, causing digital wrap-around clipping.
// Using format_MSB_Left works (I2S periph has to be set up I2S_Standard_LSB or I2S_Standard_MSB).
// Also, format_MSB_Right does not seem to work at all (with the I2S set to LSB or MSB)

void register_codec_isr(void f());

template<int sample_rate, int block_size, class T>
struct Codec : Nocopy {

  Codec() {
    instance_ = this;
    register_codec_isr(handler__IN_ITCM_);

    // Setup PLL clock for codec
    init_SAI_clock();

    //De-init the codec to force it to reset
    i2c_.DeInit();

    //Start Codec I2C
    gpio_.Init();
    i2c_.Init();

    //Start Codec SAI
    SAI_init();
    init_audio_DMA();
  }

  void Start() {
    HAL_NVIC_EnableIRQ(CODEC_SAI_RX_DMA_IRQn);
  }

private:

  DMA_HandleTypeDef hdma_rx;
  DMA_HandleTypeDef hdma_tx;

  SAI_HandleTypeDef hsai_rx;
  SAI_HandleTypeDef hsai_tx;

  volatile int16_t tx_buffer[block_size * 2 * 2];
  volatile int16_t rx_buffer[block_size * 2 * 2];

  static Codec *instance_;

  static void handler__IN_ITCM_() {
    instance_->ISR();
  }

  void ISR() {
    //Read the interrupt status register (ISR)
    uint32_t tmpisr = CODEC_SAI_RX_DMA->CODEC_SAI_RX_DMA_ISR;
    Frame *src, *dst;

    if ((tmpisr & __HAL_DMA_GET_TC_FLAG_INDEX(&instance_->hdma_rx))
        && __HAL_DMA_GET_IT_SOURCE(&instance_->hdma_rx, DMA_IT_TC)) {
      // Transfer Complete (TC) -> Point to 2nd half of buffers
      src = (Frame *)(&instance_->rx_buffer[block_size*2]);
      dst = (Frame *)(&instance_->tx_buffer[block_size*2]);
      __HAL_DMA_CLEAR_FLAG(&instance_->hdma_rx,
                           __HAL_DMA_GET_TC_FLAG_INDEX(&instance_->hdma_rx));
    } else if ((tmpisr & __HAL_DMA_GET_HT_FLAG_INDEX(&instance_->hdma_rx))
               && __HAL_DMA_GET_IT_SOURCE(&instance_->hdma_rx, DMA_IT_HT)) {
      // Half Transfer complete (HT) -> Point to 1st half of buffers
      src = (Frame *)(&instance_->rx_buffer);
      dst = (Frame *)(&instance_->tx_buffer);
      __HAL_DMA_CLEAR_FLAG(&instance_->hdma_rx,
                           __HAL_DMA_GET_HT_FLAG_INDEX(&instance_->hdma_rx));
    }

    Block<Frame, block_size> in {src};
    Block<Frame, block_size> out {dst};
    static_cast<T&>(*this).template CodecCallback<block_size>(in, out);
  }

  struct GPIO {
    // TODO init with constructor
    void Init() {
      GPIO_InitTypeDef gpio;

      CODEC_I2C_GPIO_CLOCK_ENABLE();
      CODEC_SAI_GPIO_CLOCK_ENABLE();

      //I2C pins: SDA SCL
      gpio.Mode 		= GPIO_MODE_AF_OD;
      gpio.Pull 		= GPIO_PULLUP;
      gpio.Speed 		= GPIO_SPEED_FREQ_VERY_HIGH;
      gpio.Alternate 	= CODEC_I2C_GPIO_AF;
      gpio.Pin 		= CODEC_I2C_SCL_PIN | CODEC_I2C_SDA_PIN;
      HAL_GPIO_Init(CODEC_I2C_GPIO, &gpio);

      CODEC_I2C_CLK_ENABLE();

      // SAI pins: WS, SDI, SCK, SDO
      gpio.Mode = GPIO_MODE_AF_PP;
      gpio.Pull = GPIO_NOPULL;
      gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
      gpio.Alternate 	= CODEC_SAI_GPIO_AF;

      gpio.Pin = CODEC_SAI_WS_PIN;	HAL_GPIO_Init(CODEC_SAI_GPIO_WS, &gpio);
      gpio.Pin = CODEC_SAI_SDI_PIN;	HAL_GPIO_Init(CODEC_SAI_GPIO_SDI, &gpio);
      gpio.Pin = CODEC_SAI_SCK_PIN;	HAL_GPIO_Init(CODEC_SAI_GPIO_SCK, &gpio);
      gpio.Pin = CODEC_SAI_SDO_PIN;	HAL_GPIO_Init(CODEC_SAI_GPIO_SDO, &gpio);

      if (CODEC_MCLK_SRC==MCLK_SRC_STM){

        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Pull = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio.Alternate 	= CODEC_SAI_GPIO_AF;
        gpio.Pin = CODEC_SAI_MCK_PIN;
        HAL_GPIO_Init(CODEC_SAI_MCK_GPIO, &gpio);

      } else if (CODEC_MCLK_SRC==MCLK_SRC_EXTERNAL){

        gpio.Mode = GPIO_MODE_INPUT;
        gpio.Pull = GPIO_NOPULL;
        gpio.Pin = CODEC_SAI_MCK_PIN;
        HAL_GPIO_Init(CODEC_SAI_MCK_GPIO, &gpio);
      }
    }
  } gpio_;

  struct I2C {
    void Init() {
      handle_.Instance 					= CODEC_I2C;
      handle_.Init.Timing 				= 0x20404768; //0x20445757;
      handle_.Init.OwnAddress1		 	= 0;
      handle_.Init.AddressingMode 		= I2C_ADDRESSINGMODE_7BIT;
      handle_.Init.DualAddressMode 		= I2C_DUALADDRESS_DISABLE;
      handle_.Init.OwnAddress2 			= 0;
      handle_.Init.OwnAddress2Masks		= I2C_OA2_NOMASK;
      handle_.Init.GeneralCallMode 		= I2C_GENERALCALL_DISABLE;
      handle_.Init.NoStretchMode 		= I2C_NOSTRETCH_DISABLE;

      hal_assert(HAL_I2C_Init(&handle_));
      hal_assert(HAL_I2CEx_ConfigAnalogFilter(&handle_, I2C_ANALOGFILTER_ENABLE));
      hal_assert(HAL_I2CEx_ConfigDigitalFilter(&handle_, 0));

      uint8_t i;

      uint16_t codec_init_data[] = {
        VOL_0dB,			// Reg 00: Left Line In
        VOL_0dB,			// Reg 01: Right Line In
        0b0101111,			// Reg 02: Left Headphone out (Mute)
        0b0101111,			// Reg 03: Right Headphone out (Mute)
        (MUTEMIC 			// Reg 04: Analog Audio Path Control (maximum attenuation on sidetone, sidetone disabled, DAC selected, Mute Mic, no bypass)
         | INSEL_line
         | DACSEL
         | SIDEATT_neg6dB),
        (DEEMPH_disable			// Reg 05: Digital Audio Path Control: HPF disable (no DC Blocking on input), De-emphasis disable on DAC
         | ADCHPFDisable),
        (PD_MIC
         | PD_OSC
         | PD_CLKOUT),		// Reg 06: Power Down Control (Clkout, Osc, Mic Off) 0x062
        (format_16b			// Reg 07: Digital Audio Interface Format (16-bit, slave)
         | format_I2S),
        0x000,				// Reg 08: Sampling Control (USB_NORM=Normal, BOSR=256x, default = 48k)
        0x001				// Reg 09: Active Control
      };

      // if (sample_rate==48000)					codec_init_data[WM8731_REG_SAMPLE_CTRL] |= SR_NORM_48K;
      // if (sample_rate==44100)					codec_init_data[WM8731_REG_SAMPLE_CTRL] |= SR_NORM_44K;
      // if (sample_rate==32000)					codec_init_data[WM8731_REG_SAMPLE_CTRL] |= SR_NORM_32K;
      // if (sample_rate==88200)					codec_init_data[WM8731_REG_SAMPLE_CTRL] |= SR_NORM_88K;
      // if (sample_rate==96000)					codec_init_data[WM8731_REG_SAMPLE_CTRL] |= SR_NORM_96K;
      // if (sample_rate==8000)					codec_init_data[WM8731_REG_SAMPLE_CTRL] |= SR_NORM_8K;

      Write(WM8731_REG_RESET, 0);

      for(i=0;i<W8731_NUM_REGS;++i) {
        Write(i, codec_init_data[i]);
      }
    }

    void DeInit() {
      CODEC_I2C_CLK_DISABLE();
      HAL_GPIO_DeInit(CODEC_I2C_GPIO, CODEC_I2C_SCL_PIN | CODEC_I2C_SDA_PIN);
    }

    void PowerDown() {
      Write(WM8731_REG_POWERDOWN, 0xFF); //Power Down enable all
    }

  private:
    void Write(uint8_t RegisterAddr, uint16_t RegisterValue) {
      //Assemble 2-byte data 
      uint8_t Byte1 = ((RegisterAddr<<1)&0xFE) | ((RegisterValue>>8)&0x01);
      uint8_t Byte2 = RegisterValue&0xFF;
      uint8_t data[2];

      data[0] = Byte1;
      data[1] = Byte2;

      hal_assert(HAL_I2C_Master_Transmit(&handle_, CODEC_ADDRESS, data, 2, 1000));
    }

    I2C_HandleTypeDef handle_;
  } i2c_;

  void Reboot() {
    //Take everything down...
    i2c_.PowerDown();
    i2c_.DeInit();

    DeInit_I2S_Clock();
    DeInit_SAIDMA();

    //...and bring it all back up
    this.init_SAI_clock(sample_rate);

    gpio_.Init();
    SAI_init(sample_rate);
    init_audio_DMA();

    i2c_.Init(sample_rate);

    Start();
  }

  // TODO cleanup: SAI & DMA
  void init_SAI_clock() {
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;
    PeriphClkInitStruct.PeriphClockSelection = CODEC_SAI_RCC_PERIPHCLK;

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

    PeriphClkInitStruct.CODEC_SaixClockSelection 		= CODEC_SAI_RCC_CLKSOURCE_PLLI2S;
    hal_assert(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct));
  }

  void SAI_init() {
    CODEC_SAI_CLOCK_ENABLE();

    if (!IS_SAI_AUDIO_FREQUENCY(sample_rate)) return;

    hsai_rx.Instance 				= CODEC_SAI_RX_BLOCK;
    hsai_rx.Init.AudioMode 		= SAI_MODESLAVE_RX;
    hsai_rx.Init.Synchro 			= SAI_SYNCHRONOUS;
    hsai_rx.Init.OutputDrive 		= SAI_OUTPUTDRIVE_DISABLE;
    hsai_rx.Init.FIFOThreshold 	= SAI_FIFOTHRESHOLD_EMPTY;
    hsai_rx.Init.SynchroExt 		= SAI_SYNCEXT_DISABLE;
    hsai_rx.Init.MonoStereoMode 	= SAI_STEREOMODE;
    hsai_rx.Init.CompandingMode 	= SAI_NOCOMPANDING;
    hsai_rx.Init.TriState 		= SAI_OUTPUT_NOTRELEASED;

    hsai_tx.Instance 				= CODEC_SAI_TX_BLOCK;
    hsai_tx.Init.AudioMode 		= SAI_MODEMASTER_TX;
    hsai_tx.Init.Synchro 			= SAI_ASYNCHRONOUS;
    hsai_tx.Init.OutputDrive 		= SAI_OUTPUTDRIVE_DISABLE;
    hsai_tx.Init.NoDivider 		= SAI_MASTERDIVIDER_ENABLE;
    hsai_tx.Init.FIFOThreshold 	= SAI_FIFOTHRESHOLD_EMPTY;
    hsai_tx.Init.AudioFrequency	= sample_rate;
    hsai_tx.Init.SynchroExt 		= SAI_SYNCEXT_DISABLE;
    hsai_tx.Init.MonoStereoMode 	= SAI_STEREOMODE;
    hsai_tx.Init.CompandingMode 	= SAI_NOCOMPANDING;
    hsai_tx.Init.TriState 		= SAI_OUTPUT_NOTRELEASED;

    //
    //Don't initialize them yet, we have to de-init the DMA first
    //
    HAL_SAI_DeInit(&hsai_rx);
    HAL_SAI_DeInit(&hsai_tx);
  }

  void DeInit_I2S_Clock() {
    HAL_RCCEx_DisablePLLI2S();
  }

  void DeInit_SAIDMA() {
    HAL_NVIC_DisableIRQ(CODEC_SAI_TX_DMA_IRQn); 
    HAL_NVIC_DisableIRQ(CODEC_SAI_RX_DMA_IRQn); 

    HAL_RCCEx_DisablePLLSAI();

    CODEC_SAI_CLOCK_DISABLE();

    HAL_SAI_DeInit(&hsai_rx);
    HAL_SAI_DeInit(&hsai_tx);

    HAL_DMA_Abort(&hdma_rx);
    HAL_DMA_Abort(&hdma_tx);

    HAL_DMA_DeInit(&hdma_rx);
    HAL_DMA_DeInit(&hdma_tx);
  }

  void init_audio_DMA() {
    //
    // Prepare the DMA for RX (but don't enable yet)
    //
    CODEC_SAI_DMA_CLOCK_ENABLE();

    hdma_rx.Instance = CODEC_SAI_RX_DMA_STREAM;
    hdma_rx.Init.Channel = CODEC_SAI_RX_DMA_CHANNEL;
    hdma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_rx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_rx.Init.Mode = DMA_CIRCULAR;
    hdma_rx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    hdma_rx.Init.MemBurst = DMA_MBURST_SINGLE;
    hdma_rx.Init.PeriphBurst = DMA_PBURST_SINGLE;

    hdma_tx.Instance = CODEC_SAI_TX_DMA_STREAM;
    hdma_tx.Init.Channel = CODEC_SAI_TX_DMA_CHANNEL;
    hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_tx.Init.Mode = DMA_CIRCULAR;
    hdma_tx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    hdma_tx.Init.MemBurst = DMA_MBURST_SINGLE;
    hdma_tx.Init.PeriphBurst = DMA_PBURST_SINGLE;

    HAL_DMA_DeInit(&hdma_rx);
    HAL_DMA_DeInit(&hdma_tx);

    //
    // Must initialize the SAI before initializing the DMA
    //

    hal_assert(HAL_SAI_InitProtocol(&hsai_rx, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_16BIT, 2));
    hal_assert(HAL_SAI_InitProtocol(&hsai_tx, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_16BIT, 2));

    //
    // Initialize the DMA, and link to SAI
    //

    hal_assert(HAL_DMA_Init(&hdma_rx));
    hal_assert(HAL_DMA_Init(&hdma_tx));
    __HAL_LINKDMA(&hsai_rx, hdmarx, hdma_rx);
    __HAL_LINKDMA(&hsai_tx, hdmatx, hdma_tx);

    //
    // DMA IRQ and start DMAs
    //

    HAL_NVIC_DisableIRQ(CODEC_SAI_TX_DMA_IRQn); 
    HAL_SAI_Transmit_DMA(&hsai_tx, (uint8_t *)tx_buffer, block_size * 2 * 2);

    HAL_NVIC_SetPriority(CODEC_SAI_RX_DMA_IRQn, 0, 0);
    HAL_NVIC_DisableIRQ(CODEC_SAI_RX_DMA_IRQn); 
    HAL_SAI_Receive_DMA(&hsai_rx, (uint8_t *)rx_buffer, block_size * 2 * 2);
  }

};

template<int sample_rate, int block_size, class T>
Codec<sample_rate, block_size, T> *Codec<sample_rate, block_size, T>::instance_;
