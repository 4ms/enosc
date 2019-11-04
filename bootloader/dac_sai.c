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

int32_t dac_buf[DAC_BUF_SIZE];

void (*audio_callback)(int32_t *dst, uint32_t frames);

// void placeholder_cb(int32_t *dst, uint32_t frames) {

// }

void set_dac_callback(void cb(int32_t *dst, uint32_t frames)) {
    audio_callback = cb;
}

uint8_t init_dac(void)
{
    //GPIOs A (freeze LED), C (learn LED) are already enabled
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOEEN);

    LL_GPIO_SetPinMode(DACSAI_REG_GPIO, DACSAI_REG_DATA_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(DACSAI_REG_GPIO, DACSAI_REG_LATCH_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(DACSAI_REG_GPIO, DACSAI_REG_CLK_PIN, LL_GPIO_MODE_OUTPUT);
    DAC_LATCH_HIGH;

    setup_SAI_pin(DACSAI_SAI_MCK_PIN);
    setup_SAI_pin(DACSAI_SAI_WS_PIN);
    setup_SAI_pin(DACSAI_SAI_SCK_PIN);
    setup_SAI_pin(DACSAI_SAI_SDO_PIN);

    // audio_callback = placeholder_cb;

    init_sai_clock();
    init_sai();

    init_dac_registers();

    return 1;
}
void setup_SAI_pin(uint32_t pin) {
    LL_GPIO_SetPinMode(DACSAI_SAI_GPIO, pin, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinSpeed(DACSAI_SAI_GPIO, pin, LL_GPIO_SPEED_FREQ_VERY_HIGH);
    LL_GPIO_SetAFPin_0_7(DACSAI_SAI_GPIO, pin, DACSAI_SAI_GPIO_AF);
}


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

void init_sai(void)
{
    //Enable SAI1 clock
    SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SAI1EN);
    delay(1);

    //Deinit

    //Disable all interrupts and clear all flags
    DACSAI_BLOCK->IMR = 0;
    DACSAI_BLOCK->CLRFR = 0xFFFFFFFFU;

    //Disable SAI peripheral
    DACSAI_BLOCK->CR1 &=  ~SAI_xCR1_SAIEN;
    //??while((DACSAI_BLOCK->CR1 & SAI_xCR1_SAIEN) != RESET) {;}

    // Flush the fifo
    SET_BIT(DACSAI_BLOCK->CR2, SAI_xCR2_FFLUSH);

    LL_DMA_DeInit(DMA2, LL_DMA_STREAM_1);
    
    //Disable SAI (again?)
    DACSAI_BLOCK->CR1 &=  ~SAI_xCR1_SAIEN;
    while((DACSAI_BLOCK->CR1 & SAI_xCR1_SAIEN) != RESET) {;}

    DACSAI_SAI->GCR = 0;

    const uint32_t FirstBitOffset  = 0;
    const uint32_t SlotNumber      = 2;
    const uint32_t FrameLength = 64;
    const uint32_t ActiveFrameLength = 32;
    // const uint32_t freq = 12285714; //freq = (HSE/PLLM) * I2SN / I2SQ / I2SDivQ = (16000000/16) * 344 / 4 / 7
    // const uint32_t tmpval = 4; //tmpval = freq * 10 / (AudioFreq * #Slots * 256) =  (12285714 * 10) / (48000 * 2 * 256) = (int)4.9999 = 4
    const uint32_t mckdiv = 0; // mckdiv = tmpval / 10, and it's rounded up: if ((tmpval % 10) > 8) mckdiv++;
    const uint32_t syncen_bits = 0;
    const uint32_t ckstr_bits = SAI_xCR1_CKSTR;

    DACSAI_BLOCK->CR1 &=~(SAI_xCR1_MODE | SAI_xCR1_PRTCFG |  SAI_xCR1_DS |      \
                                 SAI_xCR1_LSBFIRST | SAI_xCR1_CKSTR | SAI_xCR1_SYNCEN |\
                                 SAI_xCR1_MONO | SAI_xCR1_OUTDRIV  | SAI_xCR1_DMAEN |  \
                                 SAI_xCR1_NODIV | SAI_xCR1_MCKDIV);
    DACSAI_BLOCK->CR1 |=(SAI_MODEMASTER_TX | SAI_FREE_PROTOCOL |      \
                                SAI_DATASIZE_24 | SAI_FIRSTBIT_MSB  |        \
                                ckstr_bits | syncen_bits |                   \
                                SAI_STEREOMODE | SAI_OUTPUTDRIVE_DISABLE |   \
                                SAI_MASTERDIVIDER_ENABLE | (mckdiv << 20));

    DACSAI_BLOCK->CR2 &= ~(SAI_xCR2_FTH | SAI_xCR2_FFLUSH | SAI_xCR2_COMP | SAI_xCR2_CPL);
    DACSAI_BLOCK->CR2 |=  (SAI_FIFOTHRESHOLD_EMPTY | SAI_NOCOMPANDING | SAI_OUTPUT_NOTRELEASED);

    DACSAI_BLOCK->FRCR &=(~(SAI_xFRCR_FRL | SAI_xFRCR_FSALL | SAI_xFRCR_FSDEF | SAI_xFRCR_FSPOL | SAI_xFRCR_FSOFF));
    DACSAI_BLOCK->FRCR |=((FrameLength - 1) |
                                  SAI_FS_BEFOREFIRSTBIT |
                                  SAI_FS_CHANNEL_IDENTIFICATION |
                                  SAI_FS_ACTIVE_LOW   |
                                  ((ActiveFrameLength - 1) << 8));

    DACSAI_BLOCK->SLOTR &= (~(SAI_xSLOTR_FBOFF | SAI_xSLOTR_SLOTSZ | SAI_xSLOTR_NBSLOT | SAI_xSLOTR_SLOTEN ));
    DACSAI_BLOCK->SLOTR |=  FirstBitOffset | SAI_SLOTSIZE_32B | (SAI_SLOTACTIVE_ALL << 16) | ((SlotNumber - 1) <<  8);

	//Enable DMA clock
	LL_AHB1_GRP1_EnableClock(DACSAI_DMA_RCC);

    LL_DMA_SetChannelSelection(DACSAI_DMA, DACSAI_DMA_STREAM, DACSAI_DMA_CHANNEL);

 	LL_DMA_ConfigTransfer(DACSAI_DMA, DACSAI_DMA_STREAM, 
                                      LL_DMA_DIRECTION_MEMORY_TO_PERIPH |
                                      LL_DMA_PRIORITY_HIGH              |
                                      LL_DMA_MODE_CIRCULAR              |
                                      LL_DMA_PERIPH_NOINCREMENT         |
                                      LL_DMA_MEMORY_INCREMENT           |
                                      LL_DMA_PDATAALIGN_WORD            |
                                      LL_DMA_MDATAALIGN_WORD);
 	
    //Todo: check DAC_BUF_SIZE needs to be in bytes or halfwords or words...
	LL_DMA_SetDataLength(DACSAI_DMA, DACSAI_DMA_STREAM, DAC_BUF_SIZE); 
	LL_DMA_ConfigAddresses(DACSAI_DMA, DACSAI_DMA_STREAM, (uint32_t)&dac_buf, \
                                (uint32_t)&(DACSAI_BLOCK->DR), LL_DMA_DIRECTION_MEMORY_TO_PERIPH); 

    NVIC_SetPriority(DACSAI_DMA_IRQn, 0);
    NVIC_DisableIRQ(DACSAI_DMA_IRQn); 

    LL_DMA_EnableIT_HT(DACSAI_DMA, DACSAI_DMA_STREAM);
    LL_DMA_EnableIT_TC(DACSAI_DMA, DACSAI_DMA_STREAM);
	LL_DMA_EnableIT_TE(DACSAI_DMA, DACSAI_DMA_STREAM);

    LL_DMA_EnableStream(DACSAI_DMA, DACSAI_DMA_STREAM);

    //Enable SAI
    DACSAI_BLOCK->CR1 |= SAI_xCR1_SAIEN;
    
    //Enable SAI IT
    DACSAI_BLOCK->IMR |= SAI_xIMR_WCKCFGIE;

    //Enable SAI DMA Request
    DACSAI_BLOCK->CR1 |= SAI_xCR1_DMAEN;

}

void start_dac(void)
{
    NVIC_EnableIRQ(DACSAI_DMA_IRQn); 
}

void DACSAI_DMA_IRQHandler(void)
{
  if (LL_DMA_IsActiveFlag_TC1(DACSAI_DMA) == 1)
  {
    LL_DMA_ClearFlag_TC1(DACSAI_DMA);
    audio_callback(&(dac_buf[DAC_BLOCK_SIZE]), DAC_FRAMES_PER_BLOCK);
  }
  else if (LL_DMA_IsActiveFlag_HT1(DACSAI_DMA) == 1)
  {
    LL_DMA_ClearFlag_HT1(DACSAI_DMA);
    audio_callback(&(dac_buf[0]), DAC_FRAMES_PER_BLOCK);
  }
  else if (LL_DMA_IsActiveFlag_TE1(DACSAI_DMA) == 1)
  {
    LL_DMA_ClearFlag_TE1(DACSAI_DMA);
    //error
  }
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

    for (int i=16; i--;)
    {
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
