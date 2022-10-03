#include "hal.hh"

extern "C" {
  extern __IO uint32_t uwTick;
  void (*SysTick_ISR)();
  void RegisterSysTickISR(void f()) { SysTick_ISR = f; }
}

template<int SYSTICK_FREQ, class T>
struct System : crtp<T, System<SYSTICK_FREQ, T>>, Nocopy {
  System() {
    instance_ = this;
    SetVectorTable(0x08004000);
    HAL_DeInit();
    HAL_RCC_DeInit();

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

  inline static System* instance_;

  static void SysTickISR() {
    uwTick++;
    (**instance_).SysTickCallback();
  }

  void SetVectorTable(uint32_t reset_address) {
    SCB->VTOR = reset_address & (uint32_t)0x1FFFFF80;
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

    //Enables the Clock Security System 
    HAL_RCC_EnableCSS();

    // Configure the Systick interrupt time
    RegisterSysTickISR(&System::SysTickISR);
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/SYSTICK_FREQ);
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

namespace std {
  void __throw_bad_function_call() {
    assert_param(false);
    NVIC_SystemReset();
	while(1);
  };
}

extern "C" {
  void SysTick_Handler(void) {
    SysTick_ISR();
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

	HAL_NVIC_SystemReset();
  }
  void assert_failed(const char* file, uint32_t line) { NVIC_SystemReset(); }
  void NMI_Handler() { NVIC_SystemReset(); }
  void MemManage_Handler() { NVIC_SystemReset(); }
  void BusFault_Handler() { NVIC_SystemReset(); }
  void UsageFault_Handler() { NVIC_SystemReset(); }
  void SVC_Handler() { NVIC_SystemReset(); }
  void DebugMon_Handler() { NVIC_SystemReset(); }
  void PendSV_Handler() { NVIC_SystemReset(); }
  void __cxa_pure_virtual() { NVIC_SystemReset(); }
  __weak void _init() {}
  __weak void main() {}
}
