
#include "flash_ll.h"

void LL_FLASH_Erase_Sector(uint32_t sector, enum FlashVoltageRange voltage_range)
{
	uint32_t tmp_psize = 0;

	if (voltage_range == FLASH_VOLTAGE_RANGE_1)
		tmp_psize = FLASH_PSIZE_BYTE;

	else if (voltage_range == FLASH_VOLTAGE_RANGE_2)
		tmp_psize = FLASH_PSIZE_HALF_WORD;

	else if (voltage_range == FLASH_VOLTAGE_RANGE_3)
		tmp_psize = FLASH_PSIZE_WORD;
	else
		tmp_psize = FLASH_PSIZE_DOUBLE_WORD;

	/* If the previous operation is completed, proceed to erase the sector */
	FLASH->CR &= CR_PSIZE_MASK;
	FLASH->CR |= tmp_psize;
	FLASH->CR &= SECTOR_MASK;
	FLASH->CR |= FLASH_CR_SER | (sector << FLASH_CR_SNB_Pos);
	FLASH->CR |= FLASH_CR_STRT;

	/* Data synchronous Barrier (DSB) Just after the write operation
	This will force the CPU to respect the sequence of instruction (no optimization).*/
	__DSB();
}

void LL_FLASH_Lock(void) {
	FLASH->CR |= FLASH_CR_LOCK;
}

uint8_t LL_FLASH_Unlock(void) {
	uint8_t status=0;
	if(READ_BIT(FLASH->CR, FLASH_CR_LOCK) != 0)
	{
		WRITE_REG(FLASH->KEYR, FLASH_KEY1);
		WRITE_REG(FLASH->KEYR, FLASH_KEY2);

		if(READ_BIT(FLASH->CR, FLASH_CR_LOCK) != 0)
		 	status = 0;
	}
	return 1;
}

void LL_FLASH_Program_Word(uint32_t Address, uint32_t Data) 
{
	FLASH->CR &= CR_PSIZE_MASK;
	FLASH->CR |= FLASH_PSIZE_WORD;
	FLASH->CR |= FLASH_CR_PG;

	*(__IO uint32_t*)Address = Data;

	/* Data synchronous Barrier (DSB) Just after the write operation
	 This will force the CPU to respect the sequence of instruction (no optimization).*/
	__DSB();
}

void LL_FLASH_Program_Byte(uint32_t Address, uint8_t Data) {
	FLASH->CR &= CR_PSIZE_MASK;
	FLASH->CR |= FLASH_PSIZE_WORD;
	FLASH->CR |= FLASH_CR_PG;

	*(__IO uint8_t*)Address = Data;

	/* Data synchronous Barrier (DSB) Just after the write operation
	 This will force the CPU to respect the sequence of instruction (no optimization).*/
	__DSB();
}

void LL_FLASH_Clear_Flags(void) {
	FLASH->SR = (FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_ERSERR);
}