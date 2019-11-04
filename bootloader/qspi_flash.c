#include "qspi_flash.h"

#include "bl_utils.h" //for delay() function only

//private:
void init_QSPI_GPIO_1IO(void);
void init_QSPI_GPIO_4IO(void);


uint32_t QSPI_init(void)
{
    QSPI_CommandTypeDef s_command;

    LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_QSPI);

    CLEAR_BIT(QUADSPI->CR, QUADSPI_CR_EN);

    LL_AHB3_GRP1_ForceReset(LL_AHB3_GRP1_PERIPH_QSPI);
    LL_AHB3_GRP1_ReleaseReset(LL_AHB3_GRP1_PERIPH_QSPI);

    //Initialize chip pins in single IO mode
    init_QSPI_GPIO_1IO();

    const uint32_t ClockPrescaler     = 1; 
    const uint32_t FifoThreshold      = 16;
    const uint32_t SampleShifting     = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    const uint32_t FlashSize          = QSPI_FLASH_SIZE_ADDRESSBITS - 1;
    const uint32_t ChipSelectHighTime = QSPI_CS_HIGH_TIME_8_CYCLE; //was 1
    const uint32_t ClockMode          = QSPI_CLOCK_MODE_0;
    const uint32_t FlashID            = QSPI_FLASH_ID_2;
    const uint32_t DualFlash          = QSPI_DUALFLASH_DISABLE;

    MODIFY_REG(QUADSPI->CR, QUADSPI_CR_FTHRES, ((FifoThreshold - 1) << 8));

    LL_QSPI_WaitNotBusy();

     // Configure QSPI Clock Prescaler and Sample Shift 
    MODIFY_REG(QUADSPI->CR,(QUADSPI_CR_PRESCALER | QUADSPI_CR_SSHIFT | QUADSPI_CR_FSEL | QUADSPI_CR_DFM), 
                             ((ClockPrescaler << 24)| SampleShifting | FlashID | DualFlash));
        
     // Configure QSPI Flash Size, CS High Time and Clock Mode 
    MODIFY_REG(QUADSPI->DCR, (QUADSPI_DCR_FSIZE | QUADSPI_DCR_CSHT | QUADSPI_DCR_CKMODE), 
                            ((FlashSize << 16) | ChipSelectHighTime | ClockMode));
    
     // Enable the QSPI peripheral 
    SET_BIT(QUADSPI->CR, QUADSPI_CR_EN);

    s_command.Instruction        = RESET_ENABLE_CMD;
    s_command.AddressMode        = QSPI_ADDRESS_NONE;
    s_command.AddressSize        = 0;
    s_command.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
    s_command.AlternateBytesSize = 0;
    s_command.DataMode           = QSPI_DATA_NONE;
    s_command.DummyCycles        = 0;
    if (!LL_QSPI_Command(&s_command))
        return 0;

    s_command.Instruction = RESET_CMD;
    if (!LL_QSPI_Command(&s_command))
        return 0;

    s_command.Instruction = WRITE_ENABLE_CMD;
    if (!LL_QSPI_Command(&s_command))
        return 0;

    s_command.Instruction       = WRITE_STATUS_REG_CMD;
    s_command.DataMode          = QSPI_DATA_1_LINE;
    s_command.NbData            = 1;
    LL_QSPI_Command(&s_command);

    //Enable QPI mode
    uint8_t reg = QSPI_SR_QUADEN;
    if (!LL_QSPI_Transmit(&reg))
        return 0;

     // 40ms  Write Status/Configuration Register Cycle Time 
    delay(400);

    LL_QSPI_StartAutoPoll(QSPI_SR_QUADEN, QSPI_SR_QUADEN, 0x10, QSPI_MATCH_MODE_AND);
    uint32_t ok = LL_QSPI_WaitFlagTimeout(QSPI_FLAG_SM);
    LL_QSPI_ClearFlag(QSPI_FLAG_SM);
    if (!ok) return 0;

    // Now that chip is in QPI mode, IO2 and IO3 can be initialized
    init_QSPI_GPIO_4IO();

    return 1;
}


uint32_t QSPI_write_page(uint8_t* pData, uint32_t write_addr, uint32_t num_bytes) {
    QSPI_CommandTypeDef s_command;
    uint32_t ok;

    //Cannot write more than a page
    if (num_bytes > QSPI_PAGE_SIZE)
        return 0;

    uint32_t start_page = write_addr >> QSPI_PAGE_ADDRESS_BITS;
    uint32_t end_page = (write_addr + num_bytes - 1) >> QSPI_PAGE_ADDRESS_BITS;

    //Cannot cross page boundaries
    if (start_page != end_page)
        return 0;

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

    ok = LL_QSPI_Transmit(pData);
    if (ok) {
        LL_QSPI_StartAutoPoll(0, QSPI_SR_WIP, 0x10, QSPI_MATCH_MODE_AND);
        ok = LL_QSPI_WaitFlagTimeout(QSPI_FLAG_SM);
        LL_QSPI_ClearFlag(QSPI_FLAG_SM);
    }
    return ok;
}

uint32_t QSPI_read(uint8_t* pData, uint32_t read_addr, uint32_t num_bytes) {
    QSPI_CommandTypeDef s_command;
    uint32_t ok;

    s_command.Instruction       = QUAD_INOUT_FAST_READ_CMD;
    s_command.AddressMode       = QSPI_ADDRESS_4_LINES;
    s_command.Address           = read_addr;
    s_command.AlternateBytesSize= QSPI_ALTERNATE_BYTES_8_BITS;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES;
    s_command.AlternateBytes    = 0x00;
    s_command.DataMode          = QSPI_DATA_4_LINES;
    s_command.DummyCycles       = QSPI_DUMMY_CYCLES_READ_QUAD_IO;
    s_command.NbData            = num_bytes;

    if (!LL_QSPI_Command(&s_command))
        return 0;

    ok = LL_QSPI_Receive(pData);
            
    return ok;
}

uint32_t QSPI_erase(enum EraseCommands erase_command, uint32_t base_address) {
    QSPI_CommandTypeDef s_command;
    uint32_t ok;

    s_command.Instruction   = erase_command;
    s_command.Address       = base_address;
    s_command.AddressMode   = QSPI_ADDRESS_1_LINE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.AlternateBytesSize = 0;
    s_command.DataMode          = QSPI_DATA_NONE;
    s_command.DummyCycles       = 0;

    ok = LL_QSPI_Command(&s_command);

    if (ok) {
        LL_QSPI_StartAutoPoll(0, QSPI_SR_WIP, 0x10, QSPI_MATCH_MODE_AND);
        ok = LL_QSPI_WaitFlagTimeout(QSPI_FLAG_SM);
        LL_QSPI_ClearFlag(QSPI_FLAG_SM);
    }

    return ok;
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