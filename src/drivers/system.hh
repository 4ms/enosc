#include "hal.hh"

struct System : Nocopy {
  System() {
    SetVectorTable(0x08000000);

    HAL_Init();
    SystemClock_Config();

    SCB_InvalidateDCache();
    SCB_InvalidateICache();
    SCB_EnableICache();
    SCB_EnableDCache();

    FPU->FPDSCR |= FPU_FPDSCR_FZ_Msk;
    FPU->FPDSCR |= FPU_FPDSCR_DN_Msk;
  }

private:
  void SetVectorTable(uint32_t reset_address) {
    SCB->VTOR = reset_address & (uint32_t)0x1FFFFF80;
  }

  void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;

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
    hal_assert(HAL_RCC_OscConfig(&RCC_OscInitStruct));

    //Activate the OverDrive to reach the 216 MHz Frequency 
    hal_assert(HAL_PWREx_EnableOverDrive());

    //Initializes the CPU, AHB and APB busses clocks 
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
      RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    hal_assert(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7));

    //Note: Do not start the SAI clock (I2S) at this time.
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    PeriphClkInitStruct.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1; //54MHz

    hal_assert(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct))

    //Enables the Clock Security System 
    HAL_RCC_EnableCSS();

    // Configure the Systick interrupt time for 1ms
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

    // Some IRQs interrupt configuration
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_2);
    HAL_NVIC_SetPriority(MemoryManagement_IRQn, 0, 0);
    HAL_NVIC_SetPriority(BusFault_IRQn, 0, 0);
    HAL_NVIC_SetPriority(UsageFault_IRQn, 0, 0);
    HAL_NVIC_SetPriority(SVCall_IRQn, 0, 0);
    HAL_NVIC_SetPriority(DebugMonitor_IRQn, 0, 0);
    HAL_NVIC_SetPriority(PendSV_IRQn, 0, 0);
    HAL_NVIC_SetPriority(SysTick_IRQn, 1, 0);
  }

};

extern "C" {
  void SysTick_Handler(void) {
    HAL_IncTick();
  }
  void HardFault_Handler() {
    volatile uint32_t hfsr,dfsr,afsr,bfar,mmfar,cfsr;
    mmfar=SCB->MMFAR;
    bfar=SCB->BFAR;

    hfsr=SCB->HFSR;
    afsr=SCB->AFSR;
    dfsr=SCB->DFSR;
    cfsr=SCB->CFSR;

    UNUSED(hfsr);
    UNUSED(afsr);
    UNUSED(dfsr);
    UNUSED(cfsr);
    UNUSED(mmfar);
    UNUSED(bfar);
    
    while(1);
  }
  void assert_failed(const char* file, uint32_t line) { while (1); }
  void NMI_Handler() { while(1); }
  void MemManage_Handler() { while (1); }
  void BusFault_Handler() { while (1); }
  void UsageFault_Handler() { while (1); }
  void SVC_Handler() { while(1); }
  void DebugMon_Handler() { while(1); }
  void PendSV_Handler() { while(1); }
  __weak void _init() {}
  __weak void main() {}
}
