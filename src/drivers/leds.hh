#include "stm32f7xx.h"

#define LEARN_RED_Pin GPIO_PIN_6
#define LEARN_RED_GPIO_Port GPIOC

#define LEARN_GREEN_Pin GPIO_PIN_8
#define LEARN_GREEN_GPIO_Port GPIOC

#define LEARN_BLUE_Pin GPIO_PIN_7
#define LEARN_BLUE_GPIO_Port GPIOC


#define FREEZE_RED_Pin GPIO_PIN_8
#define FREEZE_RED_GPIO_Port GPIOA

#define FREEZE_GREEN_Pin GPIO_PIN_10
#define FREEZE_GREEN_GPIO_Port GPIOA

#define FREEZE_BLUE_Pin GPIO_PIN_9
#define FREEZE_BLUE_GPIO_Port GPIOA

#define PWM_MAX 		256	// Maximum PWM value

// PWM OUTs

#define FREEZE_LED_PWM_TIM			TIM1

#define FREEZE_LED_PWM_CC_RED 		CCR1
#define FREEZE_LED_PWM_CC_GREEN 	CCR2
#define FREEZE_LED_PWM_CC_BLUE 		CCR3

#define FREEZE_LED_PWM_CHAN_RED 	TIM_CHANNEL_1
#define FREEZE_LED_PWM_CHAN_GREEN 	TIM_CHANNEL_2
#define FREEZE_LED_PWM_CHAN_BLUE 	TIM_CHANNEL_3

#define FREEZE_LED_PWM_TIM_AF		GPIO_AF1_TIM1
#define FREEZE_LED_PWM_pins 		(FREEZE_RED_Pin | FREEZE_GREEN_Pin | FREEZE_BLUE_Pin)
#define FREEZE_LED_PWM_GPIO 		FREEZE_RED_GPIO_Port

#define LEARN_LED_PWM_TIM			TIM3

#define LEARN_LED_PWM_CC_RED 		CCR1
#define LEARN_LED_PWM_CC_GREEN 		CCR2
#define LEARN_LED_PWM_CC_BLUE 		CCR3

#define LEARN_LED_PWM_CHAN_RED 		TIM_CHANNEL_1
#define LEARN_LED_PWM_CHAN_GREEN 	TIM_CHANNEL_2
#define LEARN_LED_PWM_CHAN_BLUE 	TIM_CHANNEL_3

#define LEARN_LED_PWM_TIM_AF		GPIO_AF2_TIM3
#define LEARN_LED_PWM_pins 			(LEARN_RED_Pin | LEARN_GREEN_Pin | LEARN_BLUE_Pin)
#define LEARN_LED_PWM_GPIO 			LEARN_RED_GPIO_Port


#define LED_PWM_TIM_GPIO_RCC_ENABLE()		__HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOC_CLK_ENABLE();
#define LED_PWM_RCC_ENABLE()				__HAL_RCC_TIM1_CLK_ENABLE(); __HAL_RCC_TIM3_CLK_ENABLE();

struct Leds {
  Leds() {
    GPIO_InitTypeDef gpio;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    gpio.Pin = LEARN_RED_Pin;		HAL_GPIO_Init(LEARN_RED_GPIO_Port, &gpio);
    gpio.Pin = LEARN_GREEN_Pin;		HAL_GPIO_Init(LEARN_GREEN_GPIO_Port, &gpio);
    gpio.Pin = LEARN_BLUE_Pin;		HAL_GPIO_Init(LEARN_BLUE_GPIO_Port, &gpio);
    gpio.Pin = FREEZE_RED_Pin;		HAL_GPIO_Init(FREEZE_RED_GPIO_Port, &gpio);
    gpio.Pin = FREEZE_GREEN_Pin;	HAL_GPIO_Init(FREEZE_GREEN_GPIO_Port, &gpio);
    gpio.Pin = FREEZE_BLUE_Pin;		HAL_GPIO_Init(FREEZE_BLUE_GPIO_Port, &gpio);
  }
};


TIM_HandleTypeDef	timFREEZELED;
TIM_HandleTypeDef	timLEARNLED;

extern uint16_t	builtin_adc1_raw[ NUM_BUILTIN_ADC1 ];

void init_led_tim_pwm(void)
{
	GPIO_InitTypeDef 	gpio;
	TIM_OC_InitTypeDef	tim_oc;

	LED_PWM_RCC_ENABLE();

	//
	// Setup GPIO for timer output pins
	//
	LED_PWM_TIM_GPIO_RCC_ENABLE();

	gpio.Mode 	= GPIO_MODE_AF_PP;
	gpio.Pull 	= GPIO_PULLUP;
	gpio.Speed 	= GPIO_SPEED_FREQ_HIGH;

	gpio.Alternate 	= FREEZE_LED_PWM_TIM_AF;
	gpio.Pin 		= FREEZE_LED_PWM_pins;
	HAL_GPIO_Init(FREEZE_LED_PWM_GPIO, &gpio);

	gpio.Alternate 	= LEARN_LED_PWM_TIM_AF;
	gpio.Pin 		= LEARN_LED_PWM_pins;
	HAL_GPIO_Init(LEARN_LED_PWM_GPIO, &gpio);

	// Initialize the Timer peripherals (period determines resolution and frequency)

	timFREEZELED.Instance 				 	= FREEZE_LED_PWM_TIM;
	timFREEZELED.Init.Prescaler         	= 0;
	timFREEZELED.Init.Period            	= PWM_MAX; //216M / 1 / 256 = 840kHz;
	timFREEZELED.Init.ClockDivision     	= 0;
	timFREEZELED.Init.CounterMode       	= TIM_COUNTERMODE_UP;
	timFREEZELED.Init.RepetitionCounter 	= 0;
	timFREEZELED.Init.AutoReloadPreload 	= TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&timFREEZELED) != HAL_OK) assert_failed(__FILE__, __LINE__);


	timLEARNLED.Instance 					= LEARN_LED_PWM_TIM;
	timLEARNLED.Init.Prescaler         		= 0;
	timLEARNLED.Init.Period            		= PWM_MAX; //216M / 2 / 256 = 420kHz;
	timLEARNLED.Init.ClockDivision     		= 0;
	timLEARNLED.Init.CounterMode       		= TIM_COUNTERMODE_UP;
	timLEARNLED.Init.RepetitionCounter 		= 0;
	timLEARNLED.Init.AutoReloadPreload 		= TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_PWM_Init(&timLEARNLED) != HAL_OK) assert_failed(__FILE__, __LINE__);


	// Configure each TIMx peripheral's Output Compare units.
	// Each channel (CCRx) needs to be enabled for each TIMx that we're using

	//Common configuration for all channels
	tim_oc.OCMode       = TIM_OCMODE_PWM1;
	tim_oc.OCPolarity   = TIM_OCPOLARITY_HIGH;
	tim_oc.OCFastMode   = TIM_OCFAST_DISABLE;
	tim_oc.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
	tim_oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	tim_oc.OCIdleState  = TIM_OCIDLESTATE_RESET;
	tim_oc.Pulse 		= 0;

	if (HAL_TIM_PWM_ConfigChannel(&timFREEZELED, &tim_oc, 	FREEZE_LED_PWM_CHAN_RED) != HAL_OK)		assert_failed(__FILE__, __LINE__);
	if (HAL_TIM_PWM_ConfigChannel(&timFREEZELED, &tim_oc, 	FREEZE_LED_PWM_CHAN_GREEN) != HAL_OK)	assert_failed(__FILE__, __LINE__);
	if (HAL_TIM_PWM_ConfigChannel(&timFREEZELED, &tim_oc, 	FREEZE_LED_PWM_CHAN_BLUE) != HAL_OK)	assert_failed(__FILE__, __LINE__);

	if (HAL_TIM_PWM_ConfigChannel(&timLEARNLED, &tim_oc, 	LEARN_LED_PWM_CHAN_RED) != HAL_OK)		assert_failed(__FILE__, __LINE__);
	if (HAL_TIM_PWM_ConfigChannel(&timLEARNLED, &tim_oc, 	LEARN_LED_PWM_CHAN_GREEN) != HAL_OK)	assert_failed(__FILE__, __LINE__);
	if (HAL_TIM_PWM_ConfigChannel(&timLEARNLED, &tim_oc, 	LEARN_LED_PWM_CHAN_BLUE) != HAL_OK)		assert_failed(__FILE__, __LINE__);

	//
	// Start PWM signals generation
	//
 	if (HAL_TIM_PWM_Start(&timFREEZELED, 	FREEZE_LED_PWM_CHAN_RED) != HAL_OK)						assert_failed(__FILE__, __LINE__);
 	if (HAL_TIM_PWM_Start(&timFREEZELED, 	FREEZE_LED_PWM_CHAN_GREEN) != HAL_OK)					assert_failed(__FILE__, __LINE__);
 	if (HAL_TIM_PWM_Start(&timFREEZELED, 	FREEZE_LED_PWM_CHAN_BLUE) != HAL_OK)					assert_failed(__FILE__, __LINE__);
 
 	if (HAL_TIM_PWM_Start(&timLEARNLED, 	LEARN_LED_PWM_CHAN_RED) != HAL_OK)						assert_failed(__FILE__, __LINE__);
 	if (HAL_TIM_PWM_Start(&timLEARNLED, 	LEARN_LED_PWM_CHAN_GREEN) != HAL_OK)					assert_failed(__FILE__, __LINE__);
 	if (HAL_TIM_PWM_Start(&timLEARNLED, 	LEARN_LED_PWM_CHAN_BLUE) != HAL_OK)						assert_failed(__FILE__, __LINE__);

}

void update_led_tim_pwm(void)
{
	uint8_t ledpwm[6]={0};
	uint8_t i;

	for (i=0;i<6;i++) ledpwm[i] = builtin_adc1_raw[i]/(4096/PWM_MAX);

	FREEZE_LED_PWM_TIM->FREEZE_LED_PWM_CC_RED 	= ledpwm[0];
	FREEZE_LED_PWM_TIM->FREEZE_LED_PWM_CC_GREEN = ledpwm[1];
	FREEZE_LED_PWM_TIM->FREEZE_LED_PWM_CC_BLUE 	= ledpwm[2];

	LEARN_LED_PWM_TIM->LEARN_LED_PWM_CC_RED 	= ledpwm[3];
	LEARN_LED_PWM_TIM->LEARN_LED_PWM_CC_GREEN 	= ledpwm[4];
	LEARN_LED_PWM_TIM->LEARN_LED_PWM_CC_BLUE 	= ledpwm[5];
}
