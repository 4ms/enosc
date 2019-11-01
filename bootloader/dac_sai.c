#include "dac_sai.h"
#include "bl_utils.h"
#include "stm32f7xx_ll_dma.h"
#include "stm32f7xx_ll_bus.h"

void setup_SAI_pin(uint32_t pin);
void bitbang_write(enum PCM1753Registers reg_addr, uint8_t reg_value);
void init_sai_clock(void);
void init_sai(void);
void init_dac_registers();
void start_dac();

// __attribute__((optimize("O0"))) 

void process_audio(void) {
//placeholder
}


uint8_t init_dac(void)
{
    LL_GPIO_SetPinMode(DACSAI_REG_GPIO, DACSAI_REG_DATA_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(DACSAI_REG_GPIO, DACSAI_REG_LATCH_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(DACSAI_REG_GPIO, DACSAI_REG_CLK_PIN, LL_GPIO_MODE_OUTPUT);
    DAC_LATCH_LOW;

    setup_SAI_pin(DACSAI_SAI_MCK_PIN);
    setup_SAI_pin(DACSAI_SAI_WS_PIN);
    setup_SAI_pin(DACSAI_SAI_SCK_PIN);
    setup_SAI_pin(DACSAI_SAI_SDO_PIN);

	init_sai_clock();
	init_sai();

	init_dac_registers();

	start_dac();
}


void setup_SAI_pin(uint32_t pin) {
 	LL_GPIO_SetPinMode(DACSAI_SAI_GPIO, pin, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinSpeed(DACSAI_SAI_GPIO, pin, LL_GPIO_SPEED_FREQ_VERY_HIGH);
    LL_GPIO_SetAFPin_0_7(DACSAI_SAI_GPIO, pin, DACSAI_SAI_GPIO_AF);
}

#define RCC_FLAG_PLLI2SRDY ((uint8_t)0x3BU)
#ifndef __HAL_RCC_GET_FLAG
#define __HAL_RCC_GET_FLAG(__FLAG__) (((((((__FLAG__) >> 5) == 1)? RCC->CR :((((__FLAG__) >> 5) == 2) ? RCC->BDCR :((((__FLAG__) >> 5) == 3)? RCC->CSR :RCC->CIR))) & ((uint32_t)1 << ((__FLAG__) & ((uint8_t)0x1F))))!= 0)? 1 : 0)
#endif

void init_sai_clock(void)
{
	//48kHz:
    const uint32_t PLLI2SN_value = 344;
    const uint32_t PLLI2SQ_value = 4;
    const uint32_t PLLI2SDivQ_value = 7;

	MODIFY_REG(RCC->DCKCFGR1, RCC_DCKCFGR1_SAI1SEL, (uint32_t)RCC_DCKCFGR1_SAI1SEL_0);

	//Disable PLLI2S
    RCC->CR &= ~(RCC_CR_PLLI2SON);

    while(__HAL_RCC_GET_FLAG(RCC_FLAG_PLLI2SRDY) != RESET) {;}

	uint32_t tmpreg0 = ((RCC->PLLI2SCFGR & RCC_PLLI2SCFGR_PLLI2SR) >> RCC_PLLI2SCFGR_PLLI2SR_Pos);

    RCC->PLLI2SCFGR =  ((PLLI2SN_value) << RCC_PLLI2SCFGR_PLLI2SN_Pos) |\
					   ((PLLI2SQ_value) << RCC_PLLI2SCFGR_PLLI2SQ_Pos) |\
					   ((tmpreg0) << RCC_PLLI2SCFGR_PLLI2SR_Pos);

	MODIFY_REG(RCC->DCKCFGR1, RCC_DCKCFGR1_PLLI2SDIVQ, PLLI2SDivQ_value-1);

	//Enable PLLI2S
    RCC->CR |= (RCC_CR_PLLI2SON);
    while(__HAL_RCC_GET_FLAG(RCC_FLAG_PLLI2SRDY)  == RESET) {;}
}

#define SAI_DATASIZE_24    ((uint32_t)(SAI_xCR1_DS_2 | SAI_xCR1_DS_1))

void init_sai(void)
{
	//Enable SAI1 clock
	SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SAI1EN);
	delay(1);

    //Deinit

	//Disable all interrupts and clear all flags
	DACSAI_SAI_TX_BLOCK->IMR = 0;
	DACSAI_SAI_TX_BLOCK->CLRFR = 0xFFFFFFFFU;

	//Disable SAI peripheral
	DACSAI_SAI_TX_BLOCK->CR1 &=  ~SAI_xCR1_SAIEN;
	//??while((DACSAI_SAI_TX_BLOCK->CR1 & SAI_xCR1_SAIEN) != RESET) {;}

	// Flush the fifo
	SET_BIT(DACSAI_SAI_TX_BLOCK->CR2, SAI_xCR2_FFLUSH);


	LL_DMA_DeInit(DMA2, LL_DMA_STREAM_1);
	// DMA2->CR &=  ~DMA_SxCR_EN;
	// DMA2->CR   = 0U;
	// DMA2->NDTR = 0U;
	// DMA2->PAR  = 0U;
	// DMA2->M0AR = 0U;
	// DMA2->M1AR = 0U;
	// DMA2->FCR  = (uint32_t)0x00000021U;
	// DMA2->LIFCR = 0x00000F40U; //stream 1 flags
    
    //Disable SAI (again?)
	DACSAI_SAI_TX_BLOCK->CR1 &=  ~SAI_xCR1_SAIEN;
	while((DACSAI_SAI_TX_BLOCK->CR1 & SAI_xCR1_SAIEN) != RESET) {;}

	SAI1->GCR = 0;

	const uint32_t FirstBitOffset  = 0;
	const uint32_t SlotNumber      = 2;
	const uint32_t FrameLength = 64;
	const uint32_t ActiveFrameLength = 32;
    // const uint32_t freq = 12285714;//(16000000/16) * 344 / 4 / 7
    // const uint32_t tmpval = 4; //4.9999 == (12285714 * 10) / (48000 * 2 * 256)
   	const uint32_t mckdiv = 0; // tmpval / 10;
    const uint32_t syncen_bits = 0;
    const uint32_t ckstr_bits = SAI_xCR1_CKSTR;

  	DACSAI_SAI_TX_BLOCK->CR1 &=~(SAI_xCR1_MODE | SAI_xCR1_PRTCFG |  SAI_xCR1_DS |      \
		                         SAI_xCR1_LSBFIRST | SAI_xCR1_CKSTR | SAI_xCR1_SYNCEN |\
		                         SAI_xCR1_MONO | SAI_xCR1_OUTDRIV  | SAI_xCR1_DMAEN |  \
		                         SAI_xCR1_NODIV | SAI_xCR1_MCKDIV);
 	DACSAI_SAI_TX_BLOCK->CR1 |=(SAI_MODEMASTER_TX | SAI_FREE_PROTOCOL |      \
		                        SAI_DATASIZE_24 | SAI_FIRSTBIT_MSB  |        \
		                        ckstr_bits | syncen_bits |                   \
		                        SAI_STEREOMODE | SAI_OUTPUTDRIVE_DISABLE |   \
		                        SAI_MASTERDIVIDER_ENABLE | (mckdiv << 20));

  	DACSAI_SAI_TX_BLOCK->CR2 &= ~(SAI_xCR2_FTH | SAI_xCR2_FFLUSH | SAI_xCR2_COMP | SAI_xCR2_CPL);
  	DACSAI_SAI_TX_BLOCK->CR2 |=  (SAI_FIFOTHRESHOLD_EMPTY | SAI_NOCOMPANDING | SAI_OUTPUT_NOTRELEASED);

  	DACSAI_SAI_TX_BLOCK->FRCR &=(~(SAI_xFRCR_FRL | SAI_xFRCR_FSALL | SAI_xFRCR_FSDEF | SAI_xFRCR_FSPOL | SAI_xFRCR_FSOFF));
  	DACSAI_SAI_TX_BLOCK->FRCR |=((FrameLength - 1) |
		                          SAI_FS_BEFOREFIRSTBIT |
		                          SAI_FS_CHANNEL_IDENTIFICATION |
		                          SAI_FS_ACTIVE_LOW   |
		                          ((ActiveFrameLength - 1) << 8));

	DACSAI_SAI_TX_BLOCK->SLOTR &= (~(SAI_xSLOTR_FBOFF | SAI_xSLOTR_SLOTSZ | SAI_xSLOTR_NBSLOT | SAI_xSLOTR_SLOTEN ));
	DACSAI_SAI_TX_BLOCK->SLOTR |=  FirstBitOffset |  SAI_SLOTSIZE_32B | (SAI_SLOTACTIVE_ALL << 16) | ((SlotNumber - 1) <<  8);

/*    
	hdma_tx.Instance                  = DACSAI_SAI_TX_DMA_STREAM;xxxx
    hdma_tx.Init.Channel              = DACSAI_SAI_TX_DMA_CHANNEL;
    hdma_tx.Init.Direction            = DMA_MEMORY_TO_PERIPH;xxx
    hdma_tx.Init.PeriphInc            = DMA_PINC_DISABLE;xxxx
    hdma_tx.Init.MemInc               = DMA_MINC_ENABLE;xxxx
    hdma_tx.Init.PeriphDataAlignment  = DMA_PDATAALIGN_WORD;xxxx
    hdma_tx.Init.MemDataAlignment     = DMA_PDATAALIGN_WORD;xxxx
    hdma_tx.Init.Mode                 = DMA_CIRCULAR;xxxx
    hdma_tx.Init.Priority             = DMA_PRIORITY_HIGH;xxx
    hdma_tx.Init.FIFOMode             = DMA_FIFOMODE_DISABLE;
    hdma_tx.Init.MemBurst             = DMA_MBURST_SINGLE;
    hdma_tx.Init.PeriphBurst          = DMA_PBURST_SINGLE;
*/

	//Enable DMA clock
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);
	// LL_DMA_SetDataTransferDirection(DMA2, LL_DMA_STREAM_0, LL_DMA_DIRECTION_MEMORY_TO_MEMORY);
	// LL_DMA_SetStreamPriorityLevel(DMA2, LL_DMA_STREAM_0, LL_DMA_PRIORITY_HIGH);
	// LL_DMA_SetMode(DMA2, LL_DMA_STREAM_0, LL_DMA_MODE_NORMAL);
	// LL_DMA_SetPeriphIncMode(DMA2, LL_DMA_STREAM_0, LL_DMA_PERIPH_INCREMENT);
	// LL_DMA_SetMemoryIncMode(DMA2, LL_DMA_STREAM_0, LL_DMA_MEMORY_INCREMENT);
	// LL_DMA_SetPeriphSize(DMA2, LL_DMA_STREAM_0, LL_DMA_PDATAALIGN_WORD);
	// LL_DMA_SetMemorySize(DMA2, LL_DMA_STREAM_0, LL_DMA_MDATAALIGN_WORD);
 	LL_DMA_ConfigTransfer(DMA2, LL_DMA_STREAM_1, LL_DMA_DIRECTION_MEMORY_TO_PERIPH |
                                              LL_DMA_PRIORITY_HIGH              |
                                              LL_DMA_MODE_CIRCULAR                |
                                              LL_DMA_PERIPH_NOINCREMENT           |
                                              LL_DMA_MEMORY_INCREMENT           |
                                              LL_DMA_PDATAALIGN_WORD            |
                                              LL_DMA_MDATAALIGN_WORD);
 	
	LL_DMA_SetDataLength(DMA2, LL_DMA_STREAM_0, BUFFER_SIZE);
	LL_DMA_ConfigAddresses(DMA2, LL_DMA_STREAM_0, (uint32_t)&aSRC_Const_Buffer, (uint32_t)&aDST_Buffer, LL_DMA_GetDataTransferDirection(DMA2, LL_DMA_STREAM_0));

	/* (3) Configure NVIC for DMA transfer complete/error interrupts */
	LL_DMA_EnableIT_TC(DMA2, LL_DMA_STREAM_0);
	LL_DMA_EnableIT_TE(DMA2, LL_DMA_STREAM_0);
	NVIC_SetPriority(DMA2_Stream0_IRQn, 0);
	NVIC_EnableIRQ(DMA2_Stream0_IRQn);

	LL_DMA_EnableStream(DMA2, DACSAI_SAI_TX_DMA_STREAM);

    HAL_DMA_Init(&hdma_tx);
    __HAL_LINKDMA(&hsai_tx, hdmatx, hdma_tx);

    // DMA IRQ and start DMA
    NVIC_SetPriority(DACSAI_SAI_TX_DMA_IRQn, 0);
    NVIC_DisableIRQ(DACSAI_SAI_TX_DMA_IRQn); 

    HAL_SAI_Transmit_DMA(&hsai_tx, arrays.tx[0], block_size * 2 * 2);

}

void DACSAI_SAI_TX_DMA_IRQHandler(void)
{
  if(LL_DMA_IsActiveFlag_TC1(DMA2) == 1)
  {
    LL_DMA_ClearFlag_TC1(DMA2);
    process_audio();
  }
  else if(LL_DMA_IsActiveFlag_TE1(DMA2) == 1)
  {
    LL_DMA_ClearFlag_TE1(DMA2);
    //error
  }
}


void start_dac(void)
{
    NVIC_EnableIRQ(DACSAI_SAI_TX_DMA_IRQn);
}

void init_dac_registers(void)
{
    bitbang_write(ATTEN_REG1, NO_ATTEN);
    bitbang_write(ATTEN_REG2, NO_ATTEN);
    bitbang_write(RST_OSMP_MUTE_REG, OVER);
    bitbang_write(DEEMP_DACEN_REG, 0);
    bitbang_write(FILTER_FORMAT_REG, FMT_I2S);
    bitbang_write(ZEROFLAG_PHASE_REG, DREV);
}

void bitbang_write(enum PCM1753Registers reg_addr, uint8_t reg_value)
{
	DAC_LATCH_HIGH;
	delay(1);

	DAC_LATCH_LOW;
	DAC_CLK_LOW;
	delay(1);

	uint16_t data = (reg_addr << 8) | reg_value;

	for (int i=16; i--;) {
		if (data & (1<<i))
		    DAC_DATA_HIGH;
		else
		    DAC_DATA_LOW;

		delay(1);

		DAC_CLK_HIGH;
		delay(1);

		DAC_CLK_LOW;
		delay(1);
	}

	DAC_LATCH_HIGH;
	delay(1);
}
