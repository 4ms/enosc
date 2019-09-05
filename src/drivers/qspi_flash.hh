
//Largely taken from CubeMX Example for QSPI_ReadWrite on the STM32F73xx DISCO

#pragma once

enum QSPIErasableSizes {
	QSPI_SECTOR,
	QSPI_BLOCK_32K,
	QSPI_BLOCK_64K,
	QSPI_ENTIRE_CHIP
};

enum QSPIFlashStatus {
	QSPI_STATUS_READY,
	QSPI_STATUS_WIP,
	QSPI_STATUS_RXING,
	QSPI_STATUS_RX_COMPLETE,
	QSPI_STATUS_TXING,
	QSPI_STATUS_TX_COMPLETE
};

enum QSPIUseInterruptFlags {
	QSPI_EXECUTE_FOREGROUND,
	QSPI_EXECUTE_BACKGROUND
};




uint8_t QSPI_is_ready(void);

uint8_t QSPI_Test(void);
uint8_t QSPI_Test_Sector(uint8_t sector_num);

uint32_t QSPI_get_64kblock_addr(uint8_t block64k_num);
uint32_t QSPI_get_32kblock_addr(uint8_t block32k_num);
uint32_t QSPI_get_sector_addr(uint8_t sector_num);

uint8_t QSPI_Init(void);
uint8_t QSPI_Reset(void);

uint8_t QSPI_Read(uint8_t* pData, uint32_t read_addr, uint32_t num_bytes, enum QSPIUseInterruptFlags use_interrupt);
uint8_t QSPI_Write(uint8_t* pData, uint32_t write_addr, uint32_t num_bytes);
uint8_t QSPI_Write_Page(uint8_t* pData, uint32_t write_addr, uint32_t num_bytes, enum QSPIUseInterruptFlags use_interrupt);

uint8_t QSPI_Erase(enum QSPIErasableSizes size, uint32_t BaseAddress, enum QSPIUseInterruptFlags use_interrupt);

