#include "bootloader.hh"
#include "bl_utils.h"
#include "flash.h"

extern const uint32_t FLASH_SECTOR_ADDRESSES[];

void *memcpy(void *dest, const void *src, size_t n)
{
    char *dp = (char *)dest;
    const char *sp = (const char *)src;
    while (n--)
        *dp++ = *sp++;
    return dest;
}

void write_flash_page(const uint8_t* data, uint32_t dst_addr, uint32_t bytes_to_write)
{

	flash_begin_open_program();

	//Erase sector if dst_addr is a sector start
	flash_open_erase_sector(dst_addr);

	flash_open_program_block_words((uint32_t *)data, dst_addr, bytes_to_write>>2);

	flash_end_open_program();

}



void SetVectorTable(uint32_t reset_address)
{ 
	SCB->VTOR = reset_address & (uint32_t)0x1FFFFF80;
}


typedef void (*EntryPoint)(void);
void JumpTo(uint32_t address) 
{
	uint32_t application_address = *(__IO uint32_t*)(address + 4);
	EntryPoint application = (EntryPoint)(application_address);
	__set_MSP(*(__IO uint32_t*)address);
	application();
}



void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_ClkInitTypeDef RCC_ClkInitStruct;

    //Configure the main internal regulator output voltage
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    //Initializes the CPU, AHB and APB busses clocks
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 432;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 2; // TODO check
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    	JumpTo(0x08000000);
		// _Error_Handler(__FILE__, __LINE__);

    //Activate the OverDrive to reach the 216 MHz Frequency 
    if (HAL_PWREx_EnableOverDrive() != HAL_OK)
    	JumpTo(0x08000000);
		// _Error_Handler(__FILE__, __LINE__);

    //Initializes the CPU, AHB and APB busses clocks 
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
      RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
    	JumpTo(0x08000000);
		// _Error_Handler(__FILE__, __LINE__);


    //Enables the Clock Security System 
    HAL_RCC_EnableCSS();

    // Configure the Systick interrupt time
    // HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/(1000*TICKS_PER_MS));
    HAL_SYSTICK_Config(21600);
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

    // Some IRQs interrupt configuration
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_2);
    HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

