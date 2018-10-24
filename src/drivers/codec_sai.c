/*
 * codec_sai.c: serial audio peripheral setup
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


#include "drivers/codec_sai.h"
#include "globals.h"
#include "hal_handlers.h"
#include "drivers/codec_i2c.h"
#include "gpio_pins.h"


//Link to the process_audio_block_codec() of the main app or the bootloader
#if IS_BOOTLOADER == 1
	#include "bootloader.h"
#else
	#include "audio_stream.h"
#endif

SAI_HandleTypeDef hsai1b_rx;
SAI_HandleTypeDef hsai1a_tx;
DMA_HandleTypeDef hdma_sai1b_rx;
DMA_HandleTypeDef hdma_sai1a_tx;

volatile int32_t tx_buffer[codec_BUFF_LEN];
volatile int32_t rx_buffer[codec_BUFF_LEN];


uint32_t tx_buffer_start, rx_buffer_start, tx_buffer_half, rx_buffer_half;

void init_SAI_clock(uint32_t sample_rate)
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
		_Error_Handler(__FILE__, __LINE__);


}

void init_audio_DMA(void)
{

	tx_buffer_start = (uint32_t)&tx_buffer;
	rx_buffer_start = (uint32_t)&rx_buffer;

	tx_buffer_half = (uint32_t)(&(tx_buffer[codec_HT_LEN]));
	rx_buffer_half = (uint32_t)(&(rx_buffer[codec_HT_LEN]));

	Init_SAIDMA();
}


void reboot_codec(uint32_t sample_rate)
{
	static uint32_t last_sample_rate;

	if (sample_rate!=44100 && sample_rate!=48000 && sample_rate!=96000)
		sample_rate = 44100;


	//Do nothing if the sample_rate did not change
	
	if (last_sample_rate != sample_rate)
	{
		last_sample_rate = sample_rate; 

		//Take everything down...
		codec_power_down();
	    codec_deinit();
	   	HAL_Delay(10);

	    DeInit_I2S_Clock();
	    DeInit_SAIDMA();
	   	HAL_Delay(10);

	   	//...and bring it all back up
		init_SAI_clock(sample_rate);

		codec_GPIO_init();
		codec_SAI_init(sample_rate);
		init_audio_DMA();

		codec_I2C_init();
		codec_register_setup(sample_rate);

		start_audio();
	}

}


void start_audio(void)
{
	HAL_NVIC_EnableIRQ(CODEC_SAI_RX_DMA_IRQn); 
}

void DeInit_I2S_Clock(void)
{
	HAL_RCCEx_DisablePLLI2S();
}

void DeInit_SAIDMA(void)
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


void codec_SAI_init(uint32_t sample_rate)
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

void Init_SAIDMA(void)
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
		_Error_Handler(__FILE__, __LINE__);

	if (HAL_SAI_InitProtocol(&hsai1a_tx, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_24BIT, 2) != HAL_OK)
		_Error_Handler(__FILE__, __LINE__);

	//
	// Initialize the DMA, and link to SAI
	//

    if (HAL_DMA_Init(&hdma_sai1b_rx) != HAL_OK)
      _Error_Handler(__FILE__, __LINE__);

    __HAL_LINKDMA(&hsai1b_rx, hdmarx, hdma_sai1b_rx);

	
    if (HAL_DMA_Init(&hdma_sai1a_tx) != HAL_OK)
      _Error_Handler(__FILE__, __LINE__);

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

//DMA2_Stream5_IRQHandler
void CODEC_SAI_RX_DMA_IRQHandler(void)
{
	// HAL_DMA_IRQHandler(&hdma_sai1b_rx);


	int32_t *src, *dst;
	uint32_t err=0;
	UNUSED(err);

	//Read the interrupt status register (ISR)
	uint32_t tmpisr = CODEC_SAI_RX_DMA->CODEC_SAI_RX_DMA_ISR;

	if ((tmpisr & __HAL_DMA_GET_FE_FLAG_INDEX(&hdma_sai1b_rx)) && __HAL_DMA_GET_IT_SOURCE(&hdma_sai1b_rx, DMA_IT_FE))
	{	
		err=DMA_IT_FE; 
	}
		
	if ((tmpisr & __HAL_DMA_GET_TE_FLAG_INDEX(&hdma_sai1b_rx)) && __HAL_DMA_GET_IT_SOURCE(&hdma_sai1b_rx, DMA_IT_TE))
	{	
		err=DMA_IT_TE; 
	}

	if ((tmpisr & __HAL_DMA_GET_DME_FLAG_INDEX(&hdma_sai1b_rx)) && __HAL_DMA_GET_IT_SOURCE(&hdma_sai1b_rx, DMA_IT_DME))
	{	
		err=DMA_IT_DME; 
	}

	// Transfer Complete (TC)
	if ((tmpisr & __HAL_DMA_GET_TC_FLAG_INDEX(&hdma_sai1b_rx)) && __HAL_DMA_GET_IT_SOURCE(&hdma_sai1b_rx, DMA_IT_TC))
	{
		// Point to 2nd half of buffers
		src = (int32_t *)(rx_buffer_half);
		dst = (int32_t *)(tx_buffer_half);

		process_audio_block_codec(src, dst);

		//CODEC_SAI_RX_DMA->CODEC_SAI_RX_DMA_IFCR = CODEC_SAI_RX_DMA_FLAG_TC;
		__HAL_DMA_CLEAR_FLAG(&hdma_sai1b_rx, __HAL_DMA_GET_TC_FLAG_INDEX(&hdma_sai1b_rx));
	}

	// Half Transfer complete (HT)
	if ((tmpisr & __HAL_DMA_GET_HT_FLAG_INDEX(&hdma_sai1b_rx)) && __HAL_DMA_GET_IT_SOURCE(&hdma_sai1b_rx, DMA_IT_HT))
	{
		// Point to 1st half of buffers
		src = (int32_t *)(rx_buffer_start);
		dst = (int32_t *)(tx_buffer_start);

		process_audio_block_codec(src, dst);

		//CODEC_SAI_RX_DMA->CODEC_SAI_RX_DMA_IFCR = CODEC_SAI_RX_DMA_FLAG_HT;
		__HAL_DMA_CLEAR_FLAG(&hdma_sai1b_rx, __HAL_DMA_GET_HT_FLAG_INDEX(&hdma_sai1b_rx));
	}
}


// DMA2_Stream1_IRQHandler
// Does not get called, this is only here for debugging when enabling TX IRQ
// void CODEC_SAI_TX_DMA_IRQHandler(void)
// {
// 	HAL_DMA_IRQHandler(&hdma_sai1a_tx);
// }

