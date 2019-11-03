#include "qspi_flash.h"


void init_QSPI(void) {
    QSPI_CommandTypeDef s_command;

    LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_QSPI);

    CLEAR_BIT(QUADSPI->CR, QUADSPI_CR_EN);

    LL_AHB3_GRP1_ForceReset(LL_AHB3_GRP1_PERIPH_QSPI);
    LL_AHB3_GRP1_ReleaseReset(LL_AHB3_GRP1_PERIPH_QSPI);

    //Initialize chip pins in single IO mode
    init_QSPI_GPIO_1IO();

    //Don't use IRQ, right?
    // NVIC_SetPriority(QUADSPI_IRQn, 0b1010);
    // NVIC_EnableIRQ(QUADSPI_IRQn);

    // /* QSPI freq = SYSCLK /(1 + ClockPrescaler) = 216 MHz/(1+1) = 108 Mhz */
    const uint32_t ClockPrescaler     = 1; 
    const uint32_t FifoThreshold      = 16;
    const uint32_t SampleShifting     = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    const uint32_t FlashSize          = QSPI_FLASH_SIZE_ADDRESSBITS - 1;
    const uint32_t ChipSelectHighTime = QSPI_CS_HIGH_TIME_8_CYCLE; //was 1
    const uint32_t ClockMode          = QSPI_CLOCK_MODE_0;
    const uint32_t FlashID            = QSPI_FLASH_ID_2;
    const uint32_t DualFlash          = QSPI_DUALFLASH_DISABLE;

    MODIFY_REG(QUADSPI->CR, QUADSPI_CR_FTHRES, ((FifoThreshold - 1) << 8));

    LL_QSPI_WaitNotBusy(hqspi, QSPI_FLAG_BUSY, RESET, tickstart, hqspi->Timeout);

    // while ( (FlagStatus)(READ_BIT(QUADSPI->SR, QSPI_FLAG_BUSY)) != RESET) {;}
    while ( READ_BIT(QUADSPI->SR, QSPI_FLAG_BUSY) != 0) {;}

     // Configure QSPI Clock Prescaler and Sample Shift 
    MODIFY_REG(QUADSPI->CR,(QUADSPI_CR_PRESCALER | QUADSPI_CR_SSHIFT | QUADSPI_CR_FSEL | QUADSPI_CR_DFM), 
                             ((ClockPrescaler << 24)| SampleShifting | FlashID | DualFlash));
        
     // Configure QSPI Flash Size, CS High Time and Clock Mode 
    MODIFY_REG(QUADSPI->DCR, (QUADSPI_DCR_FSIZE | QUADSPI_DCR_CSHT | QUADSPI_DCR_CKMODE), 
                            ((FlashSize << 16) | ChipSelectHighTime | ClockMode));
    
     // Enable the QSPI peripheral 
    SET_BIT(QUADSPI->CR, QUADSPI_CR_EN);

    s_command.Instruction       = RESET_ENABLE_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AddressSize       = 0;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.AlternateBytesSize = 0;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    LL_QSPI_Command(&s_command);

    s_command.Instruction = RESET_CMD;
    LL_QSPI_Command(&s_command);

    s_command.Instruction = WRITE_ENABLE_CMD;
    LL_QSPI_Command(&s_command);

    s_command.Instruction       = WRITE_STATUS_REG_CMD;
    s_command.DataMode          = QSPI_DATA_1_LINE;
    s_command.NbData            = 1;
    LL_QSPI_Command(&s_command);

    uint8_t reg = QSPI_SR_QUADEN;
    QSPI_Transmit(&reg);

     // 40ms  Write Status/Configuration Register Cycle Time 
    delay(400);

    LL_QSPI_StartAutoPoll(QSPI_SR_QUADEN, QSPI_SR_QUADEN, 0x10, QSPI_MATCH_MODE_AND);
    LL_QSPI_WaitFlag(QSPI_FLAG_SM);
    LL_QSPI_ClearFlag(QSPI_FLAG_SM);

    // Now that chip is in QPI mode, IO2 and IO3 can be initialized
    init_QSPI_GPIO_4IO();
}


uint8_t QSPI_Write_Page(uint8_t* pData, uint32_t write_addr, uint32_t num_bytes) {
    QSPI_CommandTypeDef s_command;
    //Cannot write more than a page
    if (num_bytes > QSPI_PAGE_SIZE)
        return false;

    uint32_t start_page = write_addr >> QSPI_PAGE_ADDRESS_BITS;
    uint32_t end_page = (write_addr + num_bytes - 1) >> QSPI_PAGE_ADDRESS_BITS;

    //Cannot cross page boundaries
    if (start_page != end_page)
        return false;

    LL_QSPI_WriteEnable();

    // Initialize the program command
    s_command.Instruction       = QUAD_IN_FAST_PROG_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
    s_command.Address           = write_addr;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.AlternateBytesSize = 0;
    s_command.DataMode          = QSPI_DATA_4_LINES;
    s_command.DummyCycles       = 0;
    s_command.NbData            = num_bytes;

    LL_QSPI_Command(&s_command);

    LL_QSPI_Transmit(pData);

    LL_QSPI_StartAutoPoll(0, QSPI_SR_WIP, 0x10, QSPI_MATCH_MODE_AND);
    LL_QSPI_WaitFlag(QSPI_FLAG_SM);
    LL_QSPI_ClearFlag(QSPI_FLAG_SM);

}

uint8_t QSPI_Read(uint8_t* pData, uint32_t read_addr, uint32_t num_bytes) {

}

uint8_t QSPI_Erase(ErasableSizes size, uint32_t BaseAddress) {

}


void init_QSPI_GPIO_1IO(void) {

    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOBEN);
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOCEN);
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOEEN);


    LL_GPIO_SetPinMode(QSPI_CS_GPIO_PORT, QSPI_CS_PIN, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinSpeed(QSPI_CS_GPIO_PORT, QSPI_CS_PIN, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetAFPin_QSPI_CS();

    LL_GPIO_SetPinMode(QSPI_CLK_GPIO_PORT, QSPI_CLK_PIN, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinSpeed(QSPI_CLK_GPIO_PORT, QSPI_CLK_PIN, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetAFPin_QSPI_CLK();

    LL_GPIO_SetPinMode(QSPI_D0_GPIO_PORT, QSPI_D0_PIN, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinSpeed(QSPI_D0_GPIO_PORT, QSPI_D0_PIN, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetAFPin_QSPI_D0();

    LL_GPIO_SetPinMode(QSPI_D1_GPIO_PORT, QSPI_D1_PIN, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinSpeed(QSPI_D1_GPIO_PORT, QSPI_D1_PIN, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetAFPin_QSPI_D1();

    //D2 and D3 as high outputs to disable HOLD and WP
    LL_GPIO_SetPinMode(QSPI_D2_GPIO_PORT, QSPI_D2_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinSpeed(QSPI_D2_GPIO_PORT, QSPI_D2_PIN, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinOutputType(QSPI_D2_GPIO_PORT, QSPI_D2_PIN, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinPull(QSPI_D2_GPIO_PORT, QSPI_D2_PIN, LL_GPIO_PULL_NO);

    LL_GPIO_SetPinMode(QSPI_D3_GPIO_PORT, QSPI_D3_PIN, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinSpeed(QSPI_D3_GPIO_PORT, QSPI_D3_PIN, LL_GPIO_SPEED_FREQ_HIGH);
    LL_GPIO_SetPinOutputType(QSPI_D3_GPIO_PORT, QSPI_D3_PIN, LL_GPIO_OUTPUT_PUSHPULL); //can be combined
    LL_GPIO_SetPinPull(QSPI_D3_GPIO_PORT, QSPI_D3_PIN, LL_GPIO_PULL_NO);
    LL_GPIO_SetOutputPin(QSPI_D3_GPIO_PORT, QSPI_D2_PIN | QSPI_D3_PIN);
}

void init_QSPI_GPIO_4IO(void) {
    //Set D2 and D3 to QSPI alt function
    LL_GPIO_SetPinMode(QSPI_D2_GPIO_PORT, QSPI_D2_PIN, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_QSPI_D2();

    LL_GPIO_SetPinMode(QSPI_D3_GPIO_PORT, QSPI_D3_PIN, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_QSPI_D3();

}