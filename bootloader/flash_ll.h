#pragma once

#include <stm32f7xx.h>

enum FlashVoltageRange {
	FLASH_VOLTAGE_RANGE_1 = 0x00U,  /*!< Device operating range: 1.8V to 2.1V                */
	FLASH_VOLTAGE_RANGE_2 = 0x01U,  /*!< Device operating range: 2.1V to 2.7V                */
	FLASH_VOLTAGE_RANGE_3 = 0x02U,  /*!< Device operating range: 2.7V to 3.6V                */
	FLASH_VOLTAGE_RANGE_4 = 0x03U,  /*!< Device operating range: 2.7V to 3.6V + External Vpp */
};

#define FLASH_KEY1               ((uint32_t)0x45670123U)
#define FLASH_KEY2               ((uint32_t)0xCDEF89ABU)

#define FLASH_PSIZE_BYTE           ((uint32_t)0x00000000U)
#define FLASH_PSIZE_HALF_WORD      ((uint32_t)FLASH_CR_PSIZE_0)
#define FLASH_PSIZE_WORD           ((uint32_t)FLASH_CR_PSIZE_1)
#define FLASH_PSIZE_DOUBLE_WORD    ((uint32_t)FLASH_CR_PSIZE)
#define CR_PSIZE_MASK              ((uint32_t)0xFFFFFCFFU)
#define SECTOR_MASK               ((uint32_t)0xFFFFFF07U)

#define FLASH_FLAG_EOP                 FLASH_SR_EOP            /*!< FLASH End of Operation flag               */
#define FLASH_FLAG_OPERR               FLASH_SR_OPERR          /*!< FLASH operation Error flag                */
#define FLASH_FLAG_WRPERR              FLASH_SR_WRPERR         /*!< FLASH Write protected error flag          */
#define FLASH_FLAG_PGAERR              FLASH_SR_PGAERR         /*!< FLASH Programming Alignment error flag    */
#define FLASH_FLAG_PGPERR              FLASH_SR_PGPERR         /*!< FLASH Programming Parallelism error flag  */
#define FLASH_FLAG_ERSERR              FLASH_SR_ERSERR         /*!< FLASH Erasing Sequence error flag         */
#define FLASH_FLAG_BSY                 FLASH_SR_BSY            /*!< FLASH Busy flag                           */


void LL_FLASH_Clear_Flags(void);
void LL_FLASH_Erase_Sector(uint32_t Sector, enum FlashVoltageRange voltage_range);
void LL_FLASH_Program_Word(uint32_t Address, uint32_t Data);
void LL_FLASH_Program_Byte(uint32_t Address, uint8_t Data);
void LL_FLASH_Lock(void);
uint8_t LL_FLASH_Unlock(void);