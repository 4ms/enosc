#include "qspi_flash_driver.h"
#include "gpio_pins.h"

QSPI_HandleTypeDef QSPIHandle;
QSPI_CommandTypeDef s_command;
volatile enum QSPIFlashStatus QSPI_status = QSPI_STATUS_READY;

//private
static uint8_t QSPI_WriteEnable(void);
static void QSPI_GPIO_Init_1IO(void);
static void QSPI_GPIO_Init_IO2_IO3_AF(void);
static uint8_t QSPI_AutoPollingMemReady(uint32_t Timeout);
uint8_t QSPI_AutoPollingMemReady_IT(void);
static uint8_t QSPI_EnterMemory_QPI(void);
void QSPI_init_command(QSPI_CommandTypeDef *s_command);

uint8_t QSPI_is_ready(void) { return QSPI_status == QSPI_STATUS_READY; }
uint8_t QSPI_done_TXing(void) { return QSPI_status == QSPI_STATUS_TX_COMPLETE; }

static uint8_t test_encode_num(uint32_t num)	{return (num*7) + (num>>7);}

//
// Tests one sector
// Returns 1 if passed, 0 if failed
uint8_t QSPI_Test_Sector(uint8_t sector_num)
{
	uint32_t i;
	uint8_t test_buffer[QSPI_SECTOR_SIZE];
	uint32_t test_addr = QSPI_get_sector_addr(sector_num);

	for (i=0; i<QSPI_SECTOR_SIZE; i++)
		test_buffer[i] = test_encode_num(i);
	
	// Initiate Erasing a sector (~18us/sector)
	QSPI_Erase(QSPI_SECTOR, test_addr, QSPI_EXECUTE_BACKGROUND);
	// Free to execute code here... ~38ms
	while (!QSPI_is_ready()) {;}

	// Writing with TX and chip status polling in background:
	for (i=0; i<(QSPI_SECTOR_SIZE/QSPI_PAGE_SIZE); i++)
	{
		// Initiate a TX the data to be written (~30us/page typical)
		QSPI_Write_Page(&(test_buffer[i*QSPI_PAGE_SIZE]), test_addr+i*QSPI_PAGE_SIZE, QSPI_PAGE_SIZE, QSPI_EXECUTE_BACKGROUND); 
		// Free to execute code here... ~380us/page
		while (!QSPI_is_ready()) {;}
	}

	// Initiate a RX to read a sector: ~5us/sector
	QSPI_Read(test_buffer, test_addr, QSPI_SECTOR_SIZE, QSPI_EXECUTE_BACKGROUND);
	// Free to execute code here... ~680-850us/sector
	while (!QSPI_is_ready()) {;}

	// Verify the chip integrity by checking the data we read against our "encoding" function
	for (i=0; i<(QSPI_SECTOR_SIZE-1); i++) {
		if (test_buffer[i] != test_encode_num(i))
			return 0; //fail
	}

	return 1; //pass
}

// Tests entire chip sector-by-sector
// Returns 1 if passed, 0 if failed
uint8_t QSPI_Test(void)
{
	uint8_t i;
	for (i=0; i<QSPI_NUM_SECTORS; i++) {
		if (QSPI_Test_Sector(i)==0)
			return 0; //fail
	}
	return 1; //pass
}


uint32_t QSPI_get_64kblock_addr(uint8_t block64k_num)
{
	if (block64k_num>=QSPI_NUM_64KBLOCKS)
		return 0;

	return block64k_num * QSPI_64KBLOCK_SIZE;
}

uint32_t QSPI_get_32kblock_addr(uint8_t block32k_num)
{
	if (block32k_num>=QSPI_NUM_32KBLOCKS)
		return 0;

	return block32k_num * QSPI_32KBLOCK_SIZE;
}

uint32_t QSPI_get_sector_addr(uint8_t sector_num)
{
	if (sector_num>=QSPI_NUM_SECTORS)
		return 0;

	return sector_num * QSPI_SECTOR_SIZE;
}

uint8_t QSPI_Init(void)
{
	QSPIHandle.Instance = QUADSPI;

	if (HAL_QSPI_DeInit(&QSPIHandle) != HAL_OK)
		return HAL_ERROR;

	QSPI_CLK_ENABLE();

	QSPI_FORCE_RESET();
	QSPI_RELEASE_RESET();

	//Initialize chip pins in single IO mode
	QSPI_GPIO_Init_1IO();

	HAL_NVIC_SetPriority(QUADSPI_IRQn, 2, 2);
	HAL_NVIC_EnableIRQ(QUADSPI_IRQn);

	/* QSPI initialization */
	/* QSPI freq = SYSCLK /(1 + ClockPrescaler) = 216 MHz/(1+1) = 108 Mhz */
	QSPIHandle.Init.ClockPrescaler     = 1; 
	QSPIHandle.Init.FifoThreshold      = 16;
	QSPIHandle.Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
	QSPIHandle.Init.FlashSize          = QSPI_FLASH_SIZE_ADDRESSBITS - 1;
	QSPIHandle.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_8_CYCLE; //was 1
	QSPIHandle.Init.ClockMode          = QSPI_CLOCK_MODE_0;
	QSPIHandle.Init.FlashID            = QSPI_FLASH_ID_2;
	QSPIHandle.Init.DualFlash          = QSPI_DUALFLASH_DISABLE;

	if (HAL_QSPI_Init(&QSPIHandle) != HAL_OK)
		return HAL_ERROR;
	
	QSPI_init_command(&s_command);

	QSPI_status = QSPI_STATUS_READY;

	if (QSPI_Reset()!=HAL_OK )
		return HAL_ERROR;

	if (QSPI_EnterMemory_QPI()!=HAL_OK )
		return HAL_ERROR;

	// Now that chip is in QPI mode, IO2 and IO3 can be initialized
	QSPI_GPIO_Init_IO2_IO3_AF();

	return HAL_OK;
}

void QSPI_GPIO_Init_1IO(void)
{
	GPIO_InitTypeDef gpio;

	QSPI_CS_GPIO_CLK_ENABLE();
	QSPI_CLK_GPIO_CLK_ENABLE();
	QSPI_D0_GPIO_CLK_ENABLE();
	QSPI_D1_GPIO_CLK_ENABLE();
	QSPI_D2_GPIO_CLK_ENABLE();
	QSPI_D3_GPIO_CLK_ENABLE();

	gpio.Mode      = GPIO_MODE_AF_PP;
	gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
	gpio.Pull      = GPIO_PULLUP;

	gpio.Pin       = QSPI_CS_PIN;
	gpio.Alternate = QSPI_CS_PIN_AF;
	HAL_GPIO_Init(QSPI_CS_GPIO_PORT, &gpio);

	gpio.Pin       = QSPI_CLK_PIN;
	gpio.Alternate = QSPI_CLK_PIN_AF;
	gpio.Pull      = GPIO_NOPULL;
	HAL_GPIO_Init(QSPI_CLK_GPIO_PORT, &gpio);

	gpio.Pin       = QSPI_D0_PIN;
	gpio.Alternate = QSPI_D0_PIN_AF;
	HAL_GPIO_Init(QSPI_D0_GPIO_PORT, &gpio);

	gpio.Pin       = QSPI_D1_PIN;
	gpio.Alternate = QSPI_D1_PIN_AF;
	HAL_GPIO_Init(QSPI_D1_GPIO_PORT, &gpio);

	//IO2 and IO3 are pulled high to disable HOLD and WP
	gpio.Mode      = GPIO_MODE_OUTPUT_PP;
	gpio.Pin       = QSPI_D2_PIN;
	HAL_GPIO_Init(QSPI_D2_GPIO_PORT, &gpio);
	QSPI_D2_GPIO_PORT->BSRR = QSPI_D2_PIN;

	gpio.Mode      = GPIO_MODE_OUTPUT_PP;
	gpio.Pin       = QSPI_D3_PIN;
	HAL_GPIO_Init(QSPI_D3_GPIO_PORT, &gpio);
	QSPI_D3_GPIO_PORT->BSRR = QSPI_D3_PIN;
}

void QSPI_GPIO_Init_IO2_IO3_AF(void)
{
	GPIO_InitTypeDef gpio;

	gpio.Mode      = GPIO_MODE_AF_PP;
	gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
	gpio.Pull      = GPIO_NOPULL;

	gpio.Pin       = QSPI_D2_PIN;
	gpio.Alternate = QSPI_D2_PIN_AF;
	HAL_GPIO_Init(QSPI_D2_GPIO_PORT, &gpio);

	gpio.Pin       = QSPI_D3_PIN;
	gpio.Alternate = QSPI_D3_PIN_AF;
	HAL_GPIO_Init(QSPI_D3_GPIO_PORT, &gpio);
}

void QSPI_init_command(QSPI_CommandTypeDef *s_command)
{
	s_command->InstructionMode   = QSPI_INSTRUCTION_1_LINE;
	s_command->AddressSize       = QSPI_ADDRESS_24_BITS;
	s_command->DdrMode           = QSPI_DDR_MODE_DISABLE;
	s_command->DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	s_command->SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
}

uint8_t QSPI_Reset(void)
{
	// Enable Reset
	s_command.Instruction       = RESET_ENABLE_CMD;
	s_command.AddressMode       = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode          = QSPI_DATA_NONE;
	s_command.DummyCycles       = 0;

	if (HAL_QSPI_Command(&QSPIHandle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	// Perform Reset
	s_command.Instruction = RESET_CMD;

	if (HAL_QSPI_Command(&QSPIHandle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	return HAL_OK;
}

uint8_t QSPI_Erase(enum QSPIErasableSizes size, uint32_t BaseAddress, enum QSPIUseInterruptFlags use_interrupt)
{
	uint8_t status;
	uint32_t timeout;

	if (size==QSPI_SECTOR) {
		s_command.Instruction 	= SECTOR_ERASE_CMD;
		s_command.Address 		= BaseAddress;
		s_command.AddressMode   = QSPI_ADDRESS_1_LINE;
		timeout 				= QSPI_SECTOR_ERASE_MAX_TIME_SYSTICKS;
	}
	else if (size==QSPI_BLOCK_32K) {
		s_command.Instruction 	= BLOCK_ERASE_32K_CMD;
		s_command.Address 		= BaseAddress;
		s_command.AddressMode   = QSPI_ADDRESS_1_LINE;
		timeout 				= QSPI_32KBLOCK_ERASE_MAX_TIME_SYSTICKS;
	}
	else if (size==QSPI_BLOCK_64K) {
		s_command.Instruction 	= BLOCK_ERASE_64K_CMD;
		s_command.Address 		= BaseAddress;
		s_command.AddressMode   = QSPI_ADDRESS_1_LINE;
		timeout 				= QSPI_64KBLOCK_ERASE_MAX_TIME_SYSTICKS;
	}
	else if (size==QSPI_ENTIRE_CHIP) {
		s_command.Instruction 	= BULK_ERASE_CMD;
		s_command.Address 		= QSPI_ADDRESS_NONE;
		s_command.AddressMode   = QSPI_ADDRESS_NONE;
		timeout 				= QSPI_CHIP_ERASE_MAX_TIME_SYSTICKS;
	}
	else  
		return HAL_ERROR; //invalid size

	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode          = QSPI_DATA_NONE;
	s_command.DummyCycles       = 0;

	if (QSPI_WriteEnable() != HAL_OK)
		return HAL_ERROR;

	if (HAL_QSPI_Command(&QSPIHandle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	if (use_interrupt==QSPI_EXECUTE_BACKGROUND)
		status = QSPI_AutoPollingMemReady_IT();
	else 
		status = QSPI_AutoPollingMemReady(timeout);
	
	if (status!=HAL_OK)
		return HAL_ERROR;

	return HAL_OK;
}

uint8_t QSPI_Write(uint8_t* pData, uint32_t write_addr, uint32_t num_bytes)
{
	uint32_t end_addr, current_size, current_addr;

	// Calculation of the size between the write address and the end of the page
	current_addr = 0;

	while (current_addr <= write_addr)
		current_addr += QSPI_PAGE_SIZE;

	current_size = current_addr - write_addr;

	// Check if the size of the data is less than the remaining in the page
	if (current_size > num_bytes)
		current_size = num_bytes;

	current_addr = write_addr;
	end_addr = write_addr + num_bytes;

	s_command.Instruction       = QUAD_IN_FAST_PROG_CMD;
	s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode          = QSPI_DATA_4_LINES;
	s_command.DummyCycles       = 0;

	// Perform the write page by page
	do
	{
		s_command.Address = current_addr;
		s_command.NbData  = current_size;

		if (QSPI_WriteEnable() != HAL_OK)
			return HAL_ERROR;

		if (HAL_QSPI_Command(&QSPIHandle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
			return HAL_ERROR;

		if (HAL_QSPI_Transmit(&QSPIHandle, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
			return HAL_ERROR;

		if (QSPI_AutoPollingMemReady(HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
			return HAL_ERROR;

		current_addr += current_size;
		pData += current_size;
		current_size = ((current_addr + QSPI_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : QSPI_PAGE_SIZE;
	} while (current_addr < end_addr);

	return HAL_OK;
}

// Writes within a page (256 Bytes)
// Data to be written must not cross page boundaries.
// Setting use_interrupt to 1 means HAL_QSPI_TxCpltCallback() interrupt will be called when TX is done,
// but you must still check the chip status before accessing it again.
//
  uint8_t QSPI_Write_Page(uint8_t* pData, uint32_t write_addr, uint32_t num_bytes, enum QSPIUseInterruptFlags use_interrupt)
{
	//Cannot write more than a page
	if (num_bytes > QSPI_PAGE_SIZE)
		return HAL_ERROR;

	uint32_t start_page = write_addr >> QSPI_PAGE_ADDRESS_BITS;
	uint32_t end_page = (write_addr + num_bytes - 1) >> QSPI_PAGE_ADDRESS_BITS;

	//Cannot cross page boundaries
	if (start_page != end_page)
		return HAL_ERROR;

	// Initialize the program command
	s_command.Instruction       = QUAD_IN_FAST_PROG_CMD;
	s_command.AddressMode       = QSPI_ADDRESS_1_LINE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode          = QSPI_DATA_4_LINES;
	s_command.DummyCycles       = 0;
	s_command.Address 			= write_addr;
	s_command.NbData  			= num_bytes;
	DEBUG1_ON;
	if (QSPI_WriteEnable() != HAL_OK)
		return HAL_ERROR;

	if (HAL_QSPI_Command(&QSPIHandle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;
	DEBUG1_OFF;

	if (use_interrupt==QSPI_EXECUTE_BACKGROUND)
	{
		QSPI_status = QSPI_STATUS_TXING;

		if (HAL_QSPI_Transmit_IT(&QSPIHandle, pData) != HAL_OK)
			return HAL_ERROR;

		while (!QSPI_done_TXing()) {;}

		// Set-up auto polling, which periodically queries the chip in the background
		QSPI_AutoPollingMemReady_IT();
	}
	else
	{
		if (HAL_QSPI_Transmit(&QSPIHandle, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
			return HAL_ERROR;

		if (QSPI_AutoPollingMemReady_IT() != HAL_OK)
			return HAL_ERROR;
	}

	return HAL_OK;
}

uint8_t QSPI_Read(uint8_t* pData, uint32_t read_addr, uint32_t num_bytes, enum QSPIUseInterruptFlags use_interrupt)
{
	uint8_t status;

	//Todo: take advantage of AX Read mode (see datasheet) 
	//by setting:
	//s_command.SIOOMode			= QSPI_SIOO_INST_ONLY_FIRST_CMD;
	//s_command.AlternateByteMode 	= QSPI_ALTERNATE_BYTES_4_LINES;
	//s_command.AlternateBytesSize 	= QSPI_ALTERNATE_BYTES_8_BITS;
	//s_command.AlternateBytes 		= 0xA0;

	s_command.Instruction       = QUAD_INOUT_FAST_READ_CMD;
	s_command.AddressMode       = QSPI_ADDRESS_4_LINES;
	s_command.Address           = read_addr;
	s_command.AlternateBytesSize= QSPI_ALTERNATE_BYTES_8_BITS;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_4_LINES;
	s_command.AlternateBytes	= 0x00;
	s_command.DataMode          = QSPI_DATA_4_LINES;
	s_command.DummyCycles       = QSPI_DUMMY_CYCLES_READ_QUAD_IO;
	s_command.NbData            = num_bytes;

	if (HAL_QSPI_Command(&QSPIHandle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	if (use_interrupt==QSPI_EXECUTE_BACKGROUND) {
		QSPI_status = QSPI_STATUS_RXING;

		status = HAL_QSPI_Receive_IT(&QSPIHandle, pData);
	} else 
		status = HAL_QSPI_Receive(&QSPIHandle, pData, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
			
	if (status!=HAL_OK)
		return HAL_ERROR;

	return HAL_OK;
}

uint8_t QSPI_WriteEnable(void)
{
	QSPI_AutoPollingTypeDef s_config;

	/* Enable write operations */
	s_command.Instruction       = WRITE_ENABLE_CMD;
	s_command.AddressMode       = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode          = QSPI_DATA_NONE;
	s_command.DummyCycles       = 0;

	if (HAL_QSPI_Command(&QSPIHandle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	/* Configure automatic polling mode to wait for write enabling */
	s_config.Match           = QSPI_SR_WREN;
	s_config.Mask            = QSPI_SR_WREN;
	s_config.MatchMode       = QSPI_MATCH_MODE_AND;
	s_config.StatusBytesSize = 1;
	s_config.Interval        = 0x10;
	s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

	s_command.Instruction    = READ_STATUS_REG_CMD;
	s_command.DataMode       = QSPI_DATA_1_LINE;

	if (HAL_QSPI_AutoPolling(&QSPIHandle, &s_command, &s_config, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	return HAL_OK;
}

/**
  * @brief  Repeatedly reads the status register of the chip and waits until it indicates Write In Progress is complete.
  * @param  Timeout
  * @retval None
  */
uint8_t QSPI_AutoPollingMemReady(uint32_t Timeout)
{
	QSPI_AutoPollingTypeDef s_config;

	/* Configure automatic polling mode to wait for memory ready */
	s_command.Instruction       = READ_STATUS_REG_CMD;
	s_command.AddressMode       = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode          = QSPI_DATA_1_LINE;
	s_command.DummyCycles       = 0;

	s_config.Match           = 0;
	s_config.Mask            = QSPI_SR_WIP;
	s_config.MatchMode       = QSPI_MATCH_MODE_AND;
	s_config.StatusBytesSize = 1;
	s_config.Interval        = 0x10;
	s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

	if (HAL_QSPI_AutoPolling(&QSPIHandle, &s_command, &s_config, Timeout) != HAL_OK)
		return HAL_ERROR;

	return HAL_OK;
}

/**
  * @brief  Sets up auto-polling to call the HAL_QSPI_StatusMatchCallback() when the status register indicates Write In Progress is cleared.
  * @param  None
  * @retval None
  */

uint8_t QSPI_AutoPollingMemReady_IT(void)
{
	QSPI_AutoPollingTypeDef s_config;

	/* Configure automatic polling mode to wait for memory ready */
	s_command.Instruction       = READ_STATUS_REG_CMD;
	s_command.AddressMode       = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode          = QSPI_DATA_1_LINE;
	s_command.DummyCycles       = 0;

	s_config.Match           = 0;
	s_config.Mask            = QSPI_SR_WIP;
	s_config.MatchMode       = QSPI_MATCH_MODE_AND;
	s_config.StatusBytesSize = 1;
	s_config.Interval        = 0x10;
	s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

	QSPI_status = QSPI_STATUS_WIP;

	if (HAL_QSPI_AutoPolling_IT(&QSPIHandle, &s_command, &s_config) != HAL_OK)
		return HAL_ERROR;

	return HAL_OK;
}


/**
  * @brief  This function put QSPI memory in QPI mode (quad I/O).
  * @retval None
  */
uint8_t QSPI_EnterMemory_QPI(void)
{
	QSPI_AutoPollingTypeDef  s_config;
	uint8_t	reg;

	reg = QSPI_SR_QUADEN;

	s_command.Instruction       = WRITE_ENABLE_CMD;
	s_command.AddressMode       = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode          = QSPI_DATA_NONE;
	s_command.DummyCycles       = 0;

	if (HAL_QSPI_Command(&QSPIHandle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;


	s_command.Instruction       = WRITE_STATUS_REG_CMD;
	s_command.AddressMode       = QSPI_ADDRESS_NONE;
	s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	s_command.DataMode          = QSPI_DATA_1_LINE;
	s_command.DummyCycles       = 0;
	s_command.NbData            = 1;

	if (HAL_QSPI_Command(&QSPIHandle, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	if (HAL_QSPI_Transmit(&QSPIHandle, &reg, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	/* 40ms  Write Status/Configuration Register Cycle Time */
	HAL_Delay( 40 );

	/* Configure automatic polling mode to wait the QUADEN bit=1 and WIP bit=0 */
	s_config.Match           = QSPI_SR_QUADEN;
	s_config.Mask            = QSPI_SR_QUADEN /*|QSPI_SR_WIP*/;
	s_config.MatchMode       = QSPI_MATCH_MODE_AND;
	s_config.StatusBytesSize = 1;
	s_config.Interval        = 0x10;
	s_config.AutomaticStop   = QSPI_AUTOMATIC_STOP_ENABLE;

	s_command.Instruction    = READ_STATUS_REG_CMD;
	s_command.DataMode       = QSPI_DATA_1_LINE;

	if (HAL_QSPI_AutoPolling(&QSPIHandle, &s_command, &s_config, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
		return HAL_ERROR;

	return HAL_OK;
}


void HAL_QSPI_StatusMatchCallback(QSPI_HandleTypeDef *hqspi)
{
	QSPI_status = QSPI_STATUS_READY;
}

void HAL_QSPI_RxCpltCallback(QSPI_HandleTypeDef *hqspi)
{
	if (QSPI_status == QSPI_STATUS_RXING)
		QSPI_status = QSPI_STATUS_READY;
}

void HAL_QSPI_TxCpltCallback(QSPI_HandleTypeDef *hqspi)
{
	if (QSPI_status == QSPI_STATUS_TXING)
		QSPI_status = QSPI_STATUS_TX_COMPLETE;
}

void QUADSPI_IRQHandler(void)
{
	HAL_QSPI_IRQHandler(&QSPIHandle);
}