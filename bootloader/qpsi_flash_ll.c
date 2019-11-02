#include "qpsi_flash_ll.h"

void init_QSPI_GPIO_1IO(void);
void init_QSPI_GPIO_4IO(void);
void QSPI_init_command(QSPI_CommandTypeDef *s_command);
QSPI_CommandTypeDef s_command;

void init_QSPI(void) {

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

    //HAL_QSPI_Init(&QSPIHandle);

    MODIFY_REG(QUADSPI->CR, QUADSPI_CR_FTHRES, ((FifoThreshold - 1) << 8));

    QSPI_WaitFlagStateUntilTimeout(hqspi, QSPI_FLAG_BUSY, RESET, tickstart, hqspi->Timeout);

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

    QSPI_init_command(&s_command)


    s_command.Instruction       = RESET_ENABLE_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    //QSPI_Command(&s_command);

    // Perform Reset
    s_command.Instruction = RESET_CMD;
    //QSPI_Command(&s_command);


    QSPI_AutoPollingTypeDef  s_config;
    uint8_t reg;

    reg = QSPI_SR_QUADEN;

    s_command.Instruction       = WRITE_ENABLE_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;
    // QSPI_Command(&s_command);


    s_command.Instruction       = WRITE_STATUS_REG_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode          = QSPI_DATA_1_LINE;
    s_command.DummyCycles       = 0;
    s_command.NbData            = 1;

    // QSPI_Command(&s_command);
    // QSPI_Transmit(&reg);

     // 40ms  Write Status/Configuration Register Cycle Time 
    delay(400);

    // Configure automatic polling mode to wait the QUADEN bit=1 and WIP bit=0 
    s_config.Match           = QSPI_SR_QUADEN;
    s_config.Mask            = QSPI_SR_QUADEN /*|QSPI_SR_WIP*/;
    s_config.MatchMode       = QSPI_MATCH_MODE_AND;
    s_config.StatusBytesSize = 1;
    s_config.Interval        = 0x10;
    s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    s_command.Instruction    = READ_STATUS_REG_CMD;
    s_command.DataMode       = QSPI_DATA_1_LINE;
    // QSPI_AutoPolling(&s_command, &s_config);

    // Now that chip is in QPI mode, IO2 and IO3 can be initialized
    init_QSPI_GPIO_4IO();
}

void QSPI_init_command(QSPI_CommandTypeDef *s_command)
{
    s_command->InstructionMode   = QSPI_INSTRUCTION_1_LINE;
    s_command->AddressSize       = QSPI_ADDRESS_24_BITS;
    s_command->DdrMode           = QSPI_DDR_MODE_DISABLE;
    s_command->DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command->SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
}

void QSPI_Command(QSPI_CommandTypeDef *s_command) {


}

void QSPI_Transmit(uint8_t reg) {

}

uint8_t QSPI_WriteEnable(void)
{
    // QSPI_AutoPollingTypeDef s_config;

    // /* Enable write operations */
    // s_command.Instruction       = WRITE_ENABLE_CMD;
    // s_command.AddressMode       = QSPI_ADDRESS_NONE;
    // s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    // s_command.DataMode          = QSPI_DATA_NONE;
    // s_command.DummyCycles       = 0;

    // if (HAL_QSPI_Command(&QSPIHandle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    //     return HAL_ERROR;

    // /* Configure automatic polling mode to wait for write enabling */
    // s_config.Match           = QSPI_SR_WREN;
    // s_config.Mask            = QSPI_SR_WREN;
    // s_config.MatchMode       = QSPI_MATCH_MODE_AND;
    // s_config.StatusBytesSize = 1;
    // s_config.Interval        = 0x10;
    // s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

    // s_command.Instruction    = READ_STATUS_REG_CMD;
    // s_command.DataMode       = QSPI_DATA_1_LINE;

    // if (HAL_QSPI_AutoPolling(&QSPIHandle, &s_command, &s_config, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    //     return HAL_ERROR;

    // return HAL_OK;
}




uint8_t QSPI_Write(uint8_t* pData, uint32_t write_addr, uint32_t num_bytes) {

}

uint8_t QSPI_Read(uint8_t* pData, uint32_t read_addr, uint32_t num_bytes) {

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
