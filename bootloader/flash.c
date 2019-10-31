/*
 * flash.c -
 *
 * Author: Dan Green (danngreen1@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * See http://creativecommons.org/licenses/MIT/ for more information.
 *
 * -----------------------------------------------------------------------------
 */


#include "flash.h"
#include "flash_ll.h"

const uint32_t FLASH_SECTOR_ADDRESSES[NUM_FLASH_SECTORS+1] = {
	0x08000000, /* bootloader: 			0x08000000 - 0x08003FFF (16kB) 	*/
	0x08004000,	/* main app: 			0x08004000 - 0x0800FFFF (48kB)	*/ 	
	0x08008000,	/*   |													*/
	0x0800C000, /*   |													*/
	0x08010000, /* end of memory + 1									*/
};

void flash_erase_sector(uint32_t address)
{
	uint8_t i;

	LL_FLASH_Unlock();
	LL_FLASH_Clear_Flags();
	
	for (i = 0; i < NUM_FLASH_SECTORS; i++) {
		if (address == FLASH_SECTOR_ADDRESSES[i]) {
			LL_FLASH_Erase_Sector(i, FLASH_VOLTAGE_RANGE_3);
			break;
		}
	}
	LL_FLASH_Lock();
}

//if address is the start of a sector, erase it
//otherwise do nothing
void flash_open_erase_sector(uint32_t address)
{
	uint8_t i;

	for (i = 0; i < NUM_FLASH_SECTORS; i++) {
		if (address == FLASH_SECTOR_ADDRESSES[i]) {
		  LL_FLASH_Erase_Sector(i, FLASH_VOLTAGE_RANGE_3);
		  break;
		}
	}
}

void flash_begin_open_program(void)
{
	LL_FLASH_Unlock();
	LL_FLASH_Clear_Flags();
}

void flash_open_program_byte(uint8_t byte, uint32_t address)
{
	LL_FLASH_Program_Byte(address, byte);
}

void flash_open_program_word(uint32_t word, uint32_t address)
{
	LL_FLASH_Program_Word(address, word);
}

void flash_end_open_program(void)
{
	LL_FLASH_Lock();
}


//size is in # of bytes
void flash_open_program_block_bytes(uint8_t* arr, uint32_t address, uint32_t size)
{

	while(size--) {
		LL_FLASH_Program_Byte(address, *arr++);
		address++;
	}
}

//size is in # of 32-bit words
void flash_open_program_block_words(uint32_t* arr, uint32_t address, uint32_t size)
{
	while(size--) {
		LL_FLASH_Program_Word(address, *arr++);
		address+=4;
	}
}

//size in # of bytes
void flash_read_array(uint8_t* arr, uint32_t address, uint32_t size)
{
	while(size--) {
		*arr++ = (uint8_t)(*(__IO uint32_t*)address);
		address++;
	}
}

uint32_t flash_read_word(uint32_t address)
{
    return( *(__IO uint32_t*)address);
}

uint8_t flash_read_byte(uint32_t address)
{
    return((uint8_t) (*(__IO uint32_t*)address));
}