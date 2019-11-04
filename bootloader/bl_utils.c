#include "bootloader.hh"
#include "bl_utils.h"
#include "flash.h"
#include "gpio_pins.h"
#include "stm32f7xx_ll_system.h"
#include "stm32f7xx_ll_rcc.h"
#include "stm32f7xx_ll_pwr.h"
#include "stm32f7xx_ll_bus.h"

extern const uint32_t FLASH_SECTOR_ADDRESSES[];
extern volatile uint32_t systmr;

void init_debug(void) {
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOD);
    LL_GPIO_SetPinMode(DEBUG1_OUT_GPIO_Port, DEBUG1_OUT_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(DEBUG2_OUT_GPIO_Port, DEBUG2_OUT_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(DEBUG3_OUT_GPIO_Port, DEBUG3_OUT_Pin, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(DEBUG4_OUT_GPIO_Port, DEBUG4_OUT_Pin, LL_GPIO_MODE_OUTPUT);
}

void *memcpy(void *dest, const void *src, unsigned int n)
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

void reset_buses(void) {
    RCC->APB1RSTR = 0xFFFFFFFFU;
    RCC->APB1RSTR = 0x00U;

    RCC->APB2RSTR = 0xFFFFFFFFU;
    RCC->APB2RSTR = 0x00U;

    RCC->AHB1RSTR = 0xFFFFFFFFU;
    RCC->AHB1RSTR = 0x00U;

    RCC->AHB2RSTR = 0xFFFFFFFFU;
    RCC->AHB2RSTR = 0x00U;

    RCC->AHB3RSTR = 0xFFFFFFFFU;
    RCC->AHB3RSTR = 0x00U;

    // LL_APB1_GRP1_ForceReset(LL_APB1_GRP1_PERIPH_ALL);
    // LL_APB1_GRP1_ReleaseReset(LL_APB1_GRP1_PERIPH_ALL);

    // LL_APB2_GRP1_ForceReset(LL_APB2_GRP1_PERIPH_ALL);
    // LL_APB2_GRP1_ReleaseReset(LL_APB2_GRP1_PERIPH_ALL);

    // LL_AHB1_GRP1_ForceReset(LL_AHB1_GRP1_PERIPH_ALL);
    // LL_AHB1_GRP1_ReleaseReset(LL_AHB1_GRP1_PERIPH_ALL);

    // LL_AHB2_GRP1_ForceReset(LL_AHB2_GRP1_PERIPH_ALL);
    // LL_AHB2_GRP1_ReleaseReset(LL_AHB2_GRP1_PERIPH_ALL);

    // LL_AHB3_GRP1_ForceReset(LL_AHB3_GRP1_PERIPH_ALL);
    // LL_AHB3_GRP1_ReleaseReset(LL_AHB3_GRP1_PERIPH_ALL);

    // LL_RCC_DeInit();
}

void reset_RCC(void)
{
  SysTick->CTRL  &= ~(SysTick_CTRL_CLKSOURCE_Msk |
                   SysTick_CTRL_TICKINT_Msk   |
                   SysTick_CTRL_ENABLE_Msk);                         /* Disable SysTick IRQ and SysTick Timer */

  /* Set HSION bit to the reset value */
  SET_BIT(RCC->CR, RCC_CR_HSION);

  /* Wait till HSI is ready */
  while (READ_BIT(RCC->CR, RCC_CR_HSIRDY) == RESET) {;}

  /* Set HSITRIM[4:0] bits to the reset value */
  SET_BIT(RCC->CR, RCC_CR_HSITRIM_4);

  /* Reset CFGR register */
  CLEAR_REG(RCC->CFGR);

  /* Wait till clock switch is ready */
  while (READ_BIT(RCC->CFGR, RCC_CFGR_SWS) != RESET) {;}

  /* Clear HSEON, HSEBYP and CSSON bits */
  CLEAR_BIT(RCC->CR, RCC_CR_HSEON | RCC_CR_HSEBYP | RCC_CR_CSSON);

  /* Wait till HSE is disabled */
  while (READ_BIT(RCC->CR, RCC_CR_HSERDY) != RESET) {;}

  /* Clear PLLON bit */
  CLEAR_BIT(RCC->CR, RCC_CR_PLLON);

  /* Wait till PLL is disabled */
  while (READ_BIT(RCC->CR, RCC_CR_PLLRDY) != RESET) {;}

  /* Reset PLLI2SON bit */
  CLEAR_BIT(RCC->CR, RCC_CR_PLLI2SON);

  /* Wait till PLLI2S is disabled */
  while (READ_BIT(RCC->CR, RCC_CR_PLLI2SRDY) != RESET) {;}

  /* Reset PLLSAI bit */
  CLEAR_BIT(RCC->CR, RCC_CR_PLLSAION);

  /* Wait till PLLSAI is disabled */
  while (READ_BIT(RCC->CR, RCC_CR_PLLSAIRDY) != RESET) {;}

  /* Once PLL, PLLI2S and PLLSAI are OFF, reset PLLCFGR register to default value */
  RCC->PLLCFGR = RCC_PLLCFGR_PLLM_4 | RCC_PLLCFGR_PLLN_6 | RCC_PLLCFGR_PLLN_7 | RCC_PLLCFGR_PLLQ_2 | 0x20000000U;

  /* Reset PLLI2SCFGR register to default value */
  RCC->PLLI2SCFGR = RCC_PLLI2SCFGR_PLLI2SN_6 | RCC_PLLI2SCFGR_PLLI2SN_7 | RCC_PLLI2SCFGR_PLLI2SQ_2 | RCC_PLLI2SCFGR_PLLI2SR_1;

  /* Reset PLLSAICFGR register to default value */
  RCC->PLLSAICFGR = RCC_PLLSAICFGR_PLLSAIN_6 | RCC_PLLSAICFGR_PLLSAIN_7 | RCC_PLLSAICFGR_PLLSAIQ_2 | 0x20000000U;

  /* Disable all interrupts */
  CLEAR_BIT(RCC->CIR, RCC_CIR_LSIRDYIE | RCC_CIR_LSERDYIE | RCC_CIR_HSIRDYIE | RCC_CIR_HSERDYIE | RCC_CIR_PLLRDYIE | RCC_CIR_PLLI2SRDYIE | RCC_CIR_PLLSAIRDYIE);

  /* Clear all interrupt flags */
  SET_BIT(RCC->CIR, RCC_CIR_LSIRDYC | RCC_CIR_LSERDYC | RCC_CIR_HSIRDYC | RCC_CIR_HSERDYC | RCC_CIR_PLLRDYC | RCC_CIR_PLLI2SRDYC | RCC_CIR_PLLSAIRDYC | RCC_CIR_CSSC);

  /* Clear LSION bit */
  CLEAR_BIT(RCC->CSR, RCC_CSR_LSION);

  /* Reset all CSR flags */
  SET_BIT(RCC->CSR, RCC_CSR_RMVF);

  /* Update the SystemCoreClock global variable */
  SystemCoreClock = HSI_VALUE;
}


void delay(uint32_t ticks) {
    uint32_t i=systmr;
    while ((systmr - i) < ticks) {;}
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

void SystemClock_Config(void)
{
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);

    // LL_RCC_HSE_EnableBypass();
    LL_RCC_HSE_Enable();
    while(LL_RCC_HSE_IsReady() != 1) {;};

    LL_FLASH_SetLatency(LL_FLASH_LATENCY_7);

    LL_PWR_EnableOverDriveMode();
    while(LL_PWR_IsActiveFlag_OD() != 1) {;};

    LL_PWR_EnableOverDriveSwitching();
    while(LL_PWR_IsActiveFlag_ODSW() != 1) {;};

    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_16, 432, LL_RCC_PLLP_DIV_2);
    LL_RCC_PLL_Enable();
    while(LL_RCC_PLL_IsReady() != 1) {;};

    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {;};

    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);

    SysTick_Config(21600); //216000000/10000

    // Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function)
    SystemCoreClock = 216000000;

    NVIC_SetPriorityGrouping(5);
    NVIC_SetPriority(SysTick_IRQn, 0);
}
