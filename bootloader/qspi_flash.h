#pragma once
#include <stm32f7xx.h>
#include "qspi_flash_ll.h"


//Chip-specific:
//IS25LQ020B
#define QSPI_FLASH_SIZE_ADDRESSBITS      24     // 24 address bits = 2 Mbits
#define QSPI_FLASH_SIZE_BYTES            0x40000    // 256 KBytes
#define QSPI_64KBLOCK_SIZE               0x10000    // 64 KBytes, hence the name "64K Block" :)
#define QSPI_32KBLOCK_SIZE               0x8000     // 32 KBytes, hence the name "32K Block" :)
#define QSPI_SECTOR_SIZE                 0x1000     // 4 KBytes sectors
#define QSPI_PAGE_SIZE                   0x100      // 256 Byte pages
#define QSPI_PAGE_ADDRESS_BITS           8          // 8 bits = 256 addresses per page

#define QSPI_NUM_64KBLOCKS              (QSPI_FLASH_SIZE_BYTES/QSPI_64KBLOCK_SIZE)
#define QSPI_NUM_32KBLOCKS              (QSPI_FLASH_SIZE_BYTES/QSPI_32KBLOCK_SIZE)
#define QSPI_NUM_SECTORS                (QSPI_FLASH_SIZE_BYTES/QSPI_SECTOR_SIZE)

#define QSPI_DUMMY_CYCLES_READ           0
#define QSPI_DUMMY_CYCLES_FAST_READ      8
#define QSPI_DUMMY_CYCLES_READ_QUAD      8
#define QSPI_DUMMY_CYCLES_READ_QUAD_IO   4

//Board-specific:
#define QSPI_CS_PIN                LL_GPIO_PIN_11
#define QSPI_CS_GPIO_PORT          GPIOC
#define LL_GPIO_SetAFPin_QSPI_CS() LL_GPIO_SetAFPin_8_15(QSPI_CS_GPIO_PORT, QSPI_CS_PIN, LL_GPIO_AF_9)

#define QSPI_CLK_PIN               LL_GPIO_PIN_2
#define LL_GPIO_SetAFPin_QSPI_CLK() LL_GPIO_SetAFPin_0_7(QSPI_CS_GPIO_PORT, QSPI_CLK_PIN, LL_GPIO_AF_9)
#define QSPI_CLK_GPIO_PORT         GPIOB

#define QSPI_D0_PIN                LL_GPIO_PIN_7
#define QSPI_D0_GPIO_PORT          GPIOE
#define LL_GPIO_SetAFPin_QSPI_D0() LL_GPIO_SetAFPin_0_7(QSPI_D0_GPIO_PORT, QSPI_D0_PIN, LL_GPIO_AF_10)

#define QSPI_D1_PIN                LL_GPIO_PIN_8
#define QSPI_D1_GPIO_PORT          GPIOE
#define LL_GPIO_SetAFPin_QSPI_D1() LL_GPIO_SetAFPin_8_15(QSPI_D1_GPIO_PORT, QSPI_D1_PIN, LL_GPIO_AF_10)

#define QSPI_D2_PIN                LL_GPIO_PIN_9
#define QSPI_D2_GPIO_PORT          GPIOE
#define LL_GPIO_SetAFPin_QSPI_D2() LL_GPIO_SetAFPin_8_15(QSPI_D2_GPIO_PORT, QSPI_D2_PIN, LL_GPIO_AF_10)

#define QSPI_D3_PIN                LL_GPIO_PIN_10
#define QSPI_D3_GPIO_PORT          GPIOE
#define LL_GPIO_SetAFPin_QSPI_D3() LL_GPIO_SetAFPin_8_15(QSPI_D3_GPIO_PORT, QSPI_D3_PIN, LL_GPIO_AF_10)


uint32_t QSPI_init(void);
uint32_t QSPI_erase(enum EraseCommands erase_command, uint32_t base_address);
uint32_t QSPI_read(uint8_t* pData, uint32_t read_addr, uint32_t num_bytes);
uint32_t QSPI_write_page(uint8_t* pData, uint32_t write_addr, uint32_t num_bytes);
