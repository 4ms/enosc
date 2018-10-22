#include "stm32f4xx.h"

struct System {

  struct SysTickCallback {
    virtual void onSysTick() = 0;
  };

  SysTickCallback *cb_ = &default_callback;

  static System *instance_;
  System() {
    SystemClock_Config();
    HAL_Init();
    // SysTick takes priority over everything
    HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
    HAL_SuspendTick();
  }

  System(SysTickCallback *cb) : cb_(cb) {
    instance_ = this;
    System();
    HAL_ResumeTick();
  }

  struct DefaultCallback : SysTickCallback {
    void onSysTick() {}
  } default_callback;

  uint32_t milliseconds() { return HAL_GetTick(); }

  /**
   * @brief  System Clock Configuration
   *         The system Clock is configured as follow :
   *            System Clock source            = PLL (HSE)
   *            SYSCLK(Hz)                     = 168000000
   *            HCLK(Hz)                       = 168000000
   *            AHB Prescaler                  = 1
   *            APB1 Prescaler                 = 4
   *            APB2 Prescaler                 = 2
   *            HSE Frequency(Hz)              = 8000000
   *            PLL_M                          = 8
   *            PLL_N                          = 336
   *            PLL_P                          = 2
   *            PLL_Q                          = 7
   *            VDD(V)                         = 3.3
   *            Main regulator output voltage  = Scale1 mode
   *            Flash Latency(WS)              = 5
   * @param  None
   * @retval None
   */
  static void SystemClock_Config(void) {
      RCC_ClkInitTypeDef RCC_ClkInitStruct;
      RCC_OscInitTypeDef RCC_OscInitStruct;

      /* Enable Power Control clock */
      __HAL_RCC_PWR_CLK_ENABLE();
  
      /* The voltage scaling allows optimizing the power consumption when the device is 
         clocked below the maximum system frequency, to update the voltage scaling value 
         regarding system frequency refer to product datasheet.  */
      __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  
      /* Enable HSE Oscillator and activate PLL with HSE as source */
      RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
      RCC_OscInitStruct.HSEState = RCC_HSE_ON;
      RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
      RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
      RCC_OscInitStruct.PLL.PLLM = 8;
      RCC_OscInitStruct.PLL.PLLN = 336;
      RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
      RCC_OscInitStruct.PLL.PLLQ = 7;
      if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { while(1); }
  
      /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
         clocks dividers */
      RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
      RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
      RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
      RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
      RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
      if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) { while(1); }

      /* STM32F405x/407x/415x/417x Revision Z devices: prefetch is supported  */
      if (HAL_GetREVID() == 0x1001)
      {
        /* Enable the Flash prefetch */
        __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
      }
    }
  
};

System *System::instance_;

extern "C" {
  void SysTick_Handler() {
    HAL_IncTick();
    System::instance_->cb_->onSysTick();
  }
  void assert_failed(uint8_t* file, uint32_t line) { while (1); }
  void HardFault_Handler() { while(1); }
  void NMI_Handler() { }
  void MemManage_Handler() { while (1); }
  void BusFault_Handler() { while (1); }
  void UsageFault_Handler() { while (1); }
  void SVC_Handler() { }
  void DebugMon_Handler() { }
  void PendSV_Handler() { }
  void __cxa_pure_virtual() { while (1); }
  __weak void _init() {}
  __weak void main() {}
}
