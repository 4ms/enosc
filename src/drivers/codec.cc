/*
 * codec_i2c.c: setup and init for WM8731 codec
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

#include "codec.hh"

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


#define CODEC_FLAG_TIMEOUT             ((uint32_t)0x1000)
#define CODEC_SHORT_TIMEOUT            ((uint32_t)(1000))
#define CODEC_LONG_TIMEOUT             ((uint32_t)(300 * CODEC_FLAG_TIMEOUT))
#define CODEC_VLONG_TIMEOUT            ((uint32_t)(30000 * CODEC_FLAG_TIMEOUT))

//
// Codec buffer size
// 

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

#define CODEC_IS_SLAVE 0
#define CODEC_IS_MASTER 1

#define MCLK_SRC_STM 0
#define MCLK_SRC_EXTERNAL 1

#define W8731_ADDR_0 0x1A
#define W8731_ADDR_1 0x1B
#define W8731_NUM_REGS 10


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
#define BYPASS			(1 << 3)	/* Bypass Enable */
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


//Set configuration here:
#define CODEC_MODE 				CODEC_IS_SLAVE
#define CODEC_MCLK_SRC 			MCLK_SRC_STM
#define CODEC_ADDRESS           (W8731_ADDR_0<<1)

uint32_t Codec::write_register(uint8_t RegisterAddr, uint16_t RegisterValue)
{	
	//Assemble 2-byte data 
	uint8_t Byte1 = ((RegisterAddr<<1)&0xFE) | ((RegisterValue>>8)&0x01);
	uint8_t Byte2 = RegisterValue&0xFF;
	uint8_t data[2];

	data[0] = Byte1;
	data[1] = Byte2;

	HAL_StatusTypeDef 	err;

	while((err = HAL_I2C_Master_Transmit(&codec_i2c, CODEC_ADDRESS, data, 2, CODEC_SHORT_TIMEOUT)) != HAL_OK)
	{
		if (HAL_I2C_GetError(&codec_i2c) != HAL_I2C_ERROR_AF)
			assert_failed(__FILE__, __LINE__);
	}

	if (err==HAL_OK) 	return 0;
	else				return 1;
}

__IO uint32_t  CODECTimeout = CODEC_LONG_TIMEOUT;   


uint32_t Codec::power_down(void)
{
	uint32_t err=0;

  err=write_register(WM8731_REG_POWERDOWN, 0xFF); //Power Down enable all
	return err;

}

uint32_t Codec::reset(uint8_t master_slave, uint32_t sample_rate)
{
	uint8_t i;
	uint32_t err=0;

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
    (format_24b			// Reg 07: Digital Audio Interface Format (24-bit, slave)
     | format_I2S),
    0x000,				// Reg 08: Sampling Control (USB_NORM=Normal, BOSR=256x, default = 48k)
    0x001				// Reg 09: Active Control
  };

  err = write_register(WM8731_REG_RESET, 0);
	

	if (sample_rate==48000)					codec_init_data[WM8731_REG_SAMPLE_CTRL] |= SR_NORM_48K;
	if (sample_rate==44100)					codec_init_data[WM8731_REG_SAMPLE_CTRL] |= SR_NORM_44K;
	if (sample_rate==32000)					codec_init_data[WM8731_REG_SAMPLE_CTRL] |= SR_NORM_32K;
	if (sample_rate==88200)					codec_init_data[WM8731_REG_SAMPLE_CTRL] |= SR_NORM_88K;
	if (sample_rate==96000)					codec_init_data[WM8731_REG_SAMPLE_CTRL] |= SR_NORM_96K;
	if (sample_rate==8000)					codec_init_data[WM8731_REG_SAMPLE_CTRL] |= SR_NORM_8K;


	for(i=0;i<W8731_NUM_REGS;i++) 
    err+=write_register(i, codec_init_data[i]);

	return err;
}

uint32_t Codec::register_setup(uint32_t sample_rate)
{
	uint32_t err = 0;

  err+=reset(CODEC_MODE, sample_rate);

	return err;
}

void Codec::GPIO_init(void)
{
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

void Codec::I2C_init(void)
{

	codec_i2c.Instance 					= CODEC_I2C;
	codec_i2c.Init.Timing 				= 0x20404768; //0x20445757;
	codec_i2c.Init.OwnAddress1		 	= 0;
	codec_i2c.Init.AddressingMode 		= I2C_ADDRESSINGMODE_7BIT;
	codec_i2c.Init.DualAddressMode 		= I2C_DUALADDRESS_DISABLE;
	codec_i2c.Init.OwnAddress2 			= 0;
	codec_i2c.Init.OwnAddress2Masks		= I2C_OA2_NOMASK;
	codec_i2c.Init.GeneralCallMode 		= I2C_GENERALCALL_DISABLE;
	codec_i2c.Init.NoStretchMode 		= I2C_NOSTRETCH_DISABLE;

	if (HAL_I2C_Init(&codec_i2c) != HAL_OK)												assert_failed(__FILE__, __LINE__);
	if (HAL_I2CEx_ConfigAnalogFilter(&codec_i2c, I2C_ANALOGFILTER_ENABLE) != HAL_OK)	assert_failed(__FILE__, __LINE__);
	if (HAL_I2CEx_ConfigDigitalFilter(&codec_i2c, 0) != HAL_OK)							assert_failed(__FILE__, __LINE__);
}

void Codec::init_audio_DMA(void)
{

	tx_buffer_start = (uint32_t)&tx_buffer;
	rx_buffer_start = (uint32_t)&rx_buffer;

	tx_buffer_half = (uint32_t)(&(tx_buffer[codec_HT_LEN]));
	rx_buffer_half = (uint32_t)(&(rx_buffer[codec_HT_LEN]));

  Codec::Init_SAIDMA();
}


void Codec::reboot(uint32_t sample_rate)
{
	static uint32_t last_sample_rate;

	if (sample_rate!=44100 && sample_rate!=48000 && sample_rate!=96000)
		sample_rate = 44100;


	//Do nothing if the sample_rate did not change
	
	if (last_sample_rate != sample_rate)
	{
		last_sample_rate = sample_rate; 

		//Take everything down...
    power_down();
    Codec::deinit();
	   	HAL_Delay(10);

	    DeInit_I2S_Clock();
	    DeInit_SAIDMA();
	   	HAL_Delay(10);

	   	//...and bring it all back up
		init_SAI_clock(sample_rate);

    GPIO_init();
    SAI_init(sample_rate);
		init_audio_DMA();

    I2C_init();
    register_setup(sample_rate);

    Start();
  }
}


void Codec::Start(void)
{
	HAL_NVIC_EnableIRQ(CODEC_SAI_RX_DMA_IRQn); 
}

void Codec::DeInit_I2S_Clock(void)
{
	HAL_RCCEx_DisablePLLI2S();
}

void Codec::DeInit_SAIDMA(void)
{
	HAL_NVIC_DisableIRQ(CODEC_SAI_TX_DMA_IRQn); 
	HAL_NVIC_DisableIRQ(CODEC_SAI_RX_DMA_IRQn); 

	//__HAL_RCC_DMA2_CLK_DISABLE();
	HAL_RCCEx_DisablePLLSAI();

	CODEC_SAI_CLOCK_DISABLE();

	HAL_SAI_DeInit(&hsai1b_rx);
	HAL_SAI_DeInit(&hsai1a_tx);

	HAL_DMA_Abort(&hdma_sai1b_rx);
	HAL_DMA_Abort(&hdma_sai1a_tx);

	HAL_DMA_DeInit(&hdma_sai1b_rx);
	HAL_DMA_DeInit(&hdma_sai1a_tx);
}


void Codec::SAI_init(uint32_t sample_rate)
{
	CODEC_SAI_CLOCK_ENABLE();

	if (!IS_SAI_AUDIO_FREQUENCY(sample_rate)) return;

	hsai1b_rx.Instance 				= CODEC_SAI_RX_BLOCK;
	hsai1b_rx.Init.AudioMode 		= SAI_MODESLAVE_RX;
	hsai1b_rx.Init.Synchro 			= SAI_SYNCHRONOUS;
	hsai1b_rx.Init.OutputDrive 		= SAI_OUTPUTDRIVE_DISABLE;
	hsai1b_rx.Init.FIFOThreshold 	= SAI_FIFOTHRESHOLD_EMPTY;
	hsai1b_rx.Init.SynchroExt 		= SAI_SYNCEXT_DISABLE;
	hsai1b_rx.Init.MonoStereoMode 	= SAI_STEREOMODE;
	hsai1b_rx.Init.CompandingMode 	= SAI_NOCOMPANDING;
	hsai1b_rx.Init.TriState 		= SAI_OUTPUT_NOTRELEASED;

	hsai1a_tx.Instance 				= CODEC_SAI_TX_BLOCK;
	hsai1a_tx.Init.AudioMode 		= SAI_MODEMASTER_TX;
	hsai1a_tx.Init.Synchro 			= SAI_ASYNCHRONOUS;
	hsai1a_tx.Init.OutputDrive 		= SAI_OUTPUTDRIVE_DISABLE;
	hsai1a_tx.Init.NoDivider 		= SAI_MASTERDIVIDER_ENABLE;
	hsai1a_tx.Init.FIFOThreshold 	= SAI_FIFOTHRESHOLD_EMPTY;
	hsai1a_tx.Init.AudioFrequency	= sample_rate;
	hsai1a_tx.Init.SynchroExt 		= SAI_SYNCEXT_DISABLE;
	hsai1a_tx.Init.MonoStereoMode 	= SAI_STEREOMODE;
	hsai1a_tx.Init.CompandingMode 	= SAI_NOCOMPANDING;
	hsai1a_tx.Init.TriState 		= SAI_OUTPUT_NOTRELEASED;

	//
	//Don't initialize them yet, we have to de-init the DMA first
	//
	HAL_SAI_DeInit(&hsai1b_rx);
  HAL_SAI_DeInit(&hsai1a_tx);
}


void Codec::init_SAI_clock(uint32_t sample_rate)
{
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
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
		assert_failed(__FILE__, __LINE__);


}

void Codec::deinit(void)
{
	CODEC_I2C_CLK_DISABLE();
    HAL_GPIO_DeInit(CODEC_I2C_GPIO, CODEC_I2C_SCL_PIN | CODEC_I2C_SDA_PIN);
}


void Codec::Init_SAIDMA(void)
{
	//
	// Prepare the DMA for RX (but don't enable yet)
	//
	CODEC_SAI_DMA_CLOCK_ENABLE();

    hdma_sai1b_rx.Instance 					= CODEC_SAI_RX_DMA_STREAM;
    hdma_sai1b_rx.Init.Channel 				= CODEC_SAI_RX_DMA_CHANNEL;
    hdma_sai1b_rx.Init.Direction 			= DMA_PERIPH_TO_MEMORY;
    hdma_sai1b_rx.Init.PeriphInc 			= DMA_PINC_DISABLE;
    hdma_sai1b_rx.Init.MemInc 				= DMA_MINC_ENABLE;
    hdma_sai1b_rx.Init.PeriphDataAlignment 	= DMA_PDATAALIGN_WORD;
    hdma_sai1b_rx.Init.MemDataAlignment 	= DMA_MDATAALIGN_WORD;
    hdma_sai1b_rx.Init.Mode 				= DMA_CIRCULAR;
    hdma_sai1b_rx.Init.Priority 			= DMA_PRIORITY_HIGH;
    hdma_sai1b_rx.Init.FIFOMode 			= DMA_FIFOMODE_DISABLE;
	hdma_sai1b_rx.Init.MemBurst				= DMA_MBURST_SINGLE;
	hdma_sai1b_rx.Init.PeriphBurst			= DMA_PBURST_SINGLE; 

    hdma_sai1a_tx.Instance 					= CODEC_SAI_TX_DMA_STREAM;
    hdma_sai1a_tx.Init.Channel 				= CODEC_SAI_TX_DMA_CHANNEL;
    hdma_sai1a_tx.Init.Direction 			= DMA_MEMORY_TO_PERIPH;
    hdma_sai1a_tx.Init.PeriphInc 			= DMA_PINC_DISABLE;
    hdma_sai1a_tx.Init.MemInc 				= DMA_MINC_ENABLE;
    hdma_sai1a_tx.Init.PeriphDataAlignment 	= DMA_PDATAALIGN_WORD;
    hdma_sai1a_tx.Init.MemDataAlignment 	= DMA_MDATAALIGN_WORD;
    hdma_sai1a_tx.Init.Mode 				= DMA_CIRCULAR;
    hdma_sai1a_tx.Init.Priority 			= DMA_PRIORITY_HIGH;
    hdma_sai1a_tx.Init.FIFOMode 			= DMA_FIFOMODE_DISABLE;
   	hdma_sai1a_tx.Init.MemBurst				= DMA_MBURST_SINGLE;
	hdma_sai1a_tx.Init.PeriphBurst			= DMA_PBURST_SINGLE; 

	HAL_DMA_DeInit(&hdma_sai1b_rx);
	HAL_DMA_DeInit(&hdma_sai1a_tx);


	//
	// Must initialize the SAI before initializing the DMA
	//

	if (HAL_SAI_InitProtocol(&hsai1b_rx, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_24BIT, 2) != HAL_OK)
		assert_failed(__FILE__, __LINE__);

	if (HAL_SAI_InitProtocol(&hsai1a_tx, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_24BIT, 2) != HAL_OK)
		assert_failed(__FILE__, __LINE__);

	//
	// Initialize the DMA, and link to SAI
	//

    if (HAL_DMA_Init(&hdma_sai1b_rx) != HAL_OK)
      assert_failed(__FILE__, __LINE__);

    __HAL_LINKDMA(&hsai1b_rx, hdmarx, hdma_sai1b_rx);

	
    if (HAL_DMA_Init(&hdma_sai1a_tx) != HAL_OK)
      assert_failed(__FILE__, __LINE__);

    __HAL_LINKDMA(&hsai1a_tx, hdmatx, hdma_sai1a_tx);

    //
    // DMA IRQ and start DMAs
    //

	HAL_NVIC_DisableIRQ(CODEC_SAI_TX_DMA_IRQn); 
  	HAL_SAI_Transmit_DMA(&hsai1a_tx, (uint8_t *)tx_buffer, codec_BUFF_LEN);

	HAL_NVIC_SetPriority(CODEC_SAI_RX_DMA_IRQn, 0, 0);
	HAL_NVIC_DisableIRQ(CODEC_SAI_RX_DMA_IRQn); 
  HAL_SAI_Receive_DMA(&hsai1b_rx, (uint8_t *)rx_buffer, codec_BUFF_LEN);
}

Codec *Codec::instance_;

//DMA2_Stream5_IRQHandler
extern "C" void CODEC_SAI_RX_DMA_IRQHandler(void)
{
	// HAL_DMA_IRQHandler(&hdma_sai1b_rx);


	int32_t *src, *dst;
	uint32_t err=0;
	UNUSED(err);

	//Read the interrupt status register (ISR)
	uint32_t tmpisr = CODEC_SAI_RX_DMA->CODEC_SAI_RX_DMA_ISR;

  if ((tmpisr & __HAL_DMA_GET_FE_FLAG_INDEX(&Codec::instance_->hdma_sai1b_rx))
      && __HAL_DMA_GET_IT_SOURCE(&Codec::instance_->hdma_sai1b_rx, DMA_IT_FE)) {
		err=DMA_IT_FE; 
	}
		
  if ((tmpisr & __HAL_DMA_GET_TE_FLAG_INDEX(&Codec::instance_->hdma_sai1b_rx))
      && __HAL_DMA_GET_IT_SOURCE(&Codec::instance_->hdma_sai1b_rx, DMA_IT_TE)) {
		err=DMA_IT_TE; 
	}

  if ((tmpisr & __HAL_DMA_GET_DME_FLAG_INDEX(&Codec::instance_->hdma_sai1b_rx))
      && __HAL_DMA_GET_IT_SOURCE(&Codec::instance_->hdma_sai1b_rx, DMA_IT_DME)) {
		err=DMA_IT_DME; 
	}

	// Transfer Complete (TC)
  if ((tmpisr & __HAL_DMA_GET_TC_FLAG_INDEX(&Codec::instance_->hdma_sai1b_rx))
      && __HAL_DMA_GET_IT_SOURCE(&Codec::instance_->hdma_sai1b_rx, DMA_IT_TC)) {
		// Point to 2nd half of buffers
    src = (int32_t *)(Codec::instance_->rx_buffer_half);
    dst = (int32_t *)(Codec::instance_->tx_buffer_half);

    Codec::instance_->callback_->Process(src, dst, codec_HT_CHAN_LEN);

		//CODEC_SAI_RX_DMA->CODEC_SAI_RX_DMA_IFCR = CODEC_SAI_RX_DMA_FLAG_TC;
    __HAL_DMA_CLEAR_FLAG(&Codec::instance_->hdma_sai1b_rx,
                         __HAL_DMA_GET_TC_FLAG_INDEX(&Codec::instance_->hdma_sai1b_rx));
	}

	// Half Transfer complete (HT)
  if ((tmpisr & __HAL_DMA_GET_HT_FLAG_INDEX(&Codec::instance_->hdma_sai1b_rx))
      && __HAL_DMA_GET_IT_SOURCE(&Codec::instance_->hdma_sai1b_rx, DMA_IT_HT))
	{
		// Point to 1st half of buffers
    src = (int32_t *)(Codec::instance_->rx_buffer_start);
    dst = (int32_t *)(Codec::instance_->tx_buffer_start);

    Codec::instance_->callback_->Process(src, dst, codec_HT_CHAN_LEN);

		//CODEC_SAI_RX_DMA->CODEC_SAI_RX_DMA_IFCR = CODEC_SAI_RX_DMA_FLAG_HT;
    __HAL_DMA_CLEAR_FLAG(&Codec::instance_->hdma_sai1b_rx,
                         __HAL_DMA_GET_HT_FLAG_INDEX(&Codec::instance_->hdma_sai1b_rx));
	}
}


// DMA2_Stream1_IRQHandler
// Does not get called, this is only here for debugging when enabling TX IRQ
// void CODEC_SAI_TX_DMA_IRQHandler(void)
// {
// 	HAL_DMA_IRQHandler(&hdma_sai1a_tx);
// }
