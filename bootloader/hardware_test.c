#include "hardware_test.h"
#include "gpio_pins.h"
#include "adc.h"

void test_leds(void);
void test_switches(void);
void test_pots(void);

//bit-bangs reg init for dac
//
void test_dac(void);

//one jack at a time: read 0-8V on jacks: red goes off when 8V read, blue goes off after that and 0V read. Green goes off when center read
//press button to continue
void test_cv_jacks(void);

//checks SPI communication
//reads value from pitch/root jacks: 
//red goes off when 6V read, blue goes off after that and -2V read. Green goes off after that when 0V read
//Similar for gate jacks
void test_extadc(void);

//internal read/write/compare test: warning: erases entire chip
void test_QSPI(void);


void do_hardware_test(void) {
    test_leds();
    test_switches();
    test_pots();
}


void set_freeze_red(uint32_t val) {FREEZE_LED_PWM_TIM->FREEZE_LED_PWM_CC_RED = val;}
void set_freeze_green(uint32_t val) {FREEZE_LED_PWM_TIM->FREEZE_LED_PWM_CC_GREEN = val;}
void set_freeze_blue(uint32_t val) {FREEZE_LED_PWM_TIM->FREEZE_LED_PWM_CC_BLUE = val;}
void set_learn_red(uint32_t val) {LEARN_LED_PWM_TIM->LEARN_LED_PWM_CC_RED = val;}
void set_learn_green(uint32_t val) {LEARN_LED_PWM_TIM->LEARN_LED_PWM_CC_GREEN = val;}
void set_learn_blue(uint32_t val) {LEARN_LED_PWM_TIM->LEARN_LED_PWM_CC_BLUE = val;}

static inline uint32_t learn_pressed(void) { return PIN_READ(LEARN_BUT_GPIO_Port, LEARN_BUT_Pin); }
static inline uint32_t freeze_pressed(void) { return PIN_READ(FREEZE_BUT_GPIO_Port, FREEZE_BUT_Pin); }

void wait_for_freeze_released(void)  { while (freeze_pressed()) {;} }
void wait_for_freeze_pressed(void)  { while (!freeze_pressed()) {;} }
void wait_for_learn_released(void)  { while (learn_pressed()) {;} }
void wait_for_learn_pressed(void)  { while (!learn_pressed()) {;} }

//press buttons to turn off each led color: red green blue
void test_leds(void) {
    wait_for_learn_released();
    wait_for_freeze_released();

    set_freeze_red(255);
    set_freeze_green(255);
    set_freeze_blue(255);
    set_learn_red(255);
    set_learn_green(255);
    set_learn_blue(255);

    wait_for_learn_pressed();
    set_learn_red(0);
    wait_for_learn_released();
    wait_for_learn_pressed();
    set_learn_green(0);
    wait_for_learn_released();
    wait_for_learn_pressed();
    set_learn_blue(0);
    wait_for_learn_released();

    wait_for_freeze_pressed();
    set_freeze_red(0);
    wait_for_freeze_released();
    wait_for_freeze_pressed();
    set_freeze_green(0);
    wait_for_freeze_released();
    wait_for_freeze_pressed();
    set_freeze_blue(0);
    wait_for_freeze_released();
}

//flip switches to turn leds off: each switch must be found in 3 positions before turning an element off
//only red/blue are used (four switches, four led elements)
void test_switches(void) {
    uint32_t switch_state[4] = {0};
    GPIO_InitTypeDef gpio;

    //GPIOs A (freeze LED), B (learn jack), C (learn LED) are already enabled
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOEEN);
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIODEN);

    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;

    gpio.Pin = CROSSFMSW_TOP_Pin|CROSSFMSW_BOT_Pin;
    HAL_GPIO_Init(CROSSFMSW_TOP_GPIO_Port, &gpio);

    gpio.Pin = SCALESW_TOP_Pin|SCALESW_BOT_Pin;
    HAL_GPIO_Init(SCALESW_TOP_GPIO_Port, &gpio);

    gpio.Pin = TWISTSW_TOP_Pin|TWISTSW_BOT_Pin;
    HAL_GPIO_Init(TWISTSW_TOP_GPIO_Port, &gpio);

    gpio.Pin = WARPSW_TOP_Pin;
    HAL_GPIO_Init(WARPSW_TOP_GPIO_Port, &gpio);

    gpio.Pin = WARPSW_BOT_Pin;
    HAL_GPIO_Init(WARPSW_BOT_GPIO_Port, &gpio);

    set_freeze_red(255);
    set_freeze_blue(255);
    set_learn_red(255);
    set_learn_blue(255);
    uint32_t sw;
    uint8_t flash=0;
    while (!learn_pressed()){
        sw = PIN_READ(CROSSFMSW_TOP_GPIO_Port, CROSSFMSW_TOP_Pin) & 
                            PIN_READ(CROSSFMSW_BOT_GPIO_Port, CROSSFMSW_BOT_Pin);
        if (sw==0b11) switch_state[0] |= 1;
        if (sw==0b01) switch_state[0] |= 2;
        if (sw==0b10) switch_state[0] |= 4;
        if (sw==0b00) { set_learn_green(255); while (1) {;} }
        if (switch_state[0]==7) set_freeze_red(0); //switch passes

        sw = PIN_READ(SCALESW_TOP_GPIO_Port, SCALESW_TOP_Pin) & 
                            PIN_READ(SCALESW_BOT_GPIO_Port, SCALESW_BOT_Pin);
        if (sw==0b11) switch_state[1] |= 1;
        if (sw==0b01) switch_state[1] |= 2;
        if (sw==0b10) switch_state[1] |= 4;
        if (sw==0b00) { set_learn_green(255); while (1) {;} }
        if (switch_state[1]==7) set_learn_red(0);

        sw = PIN_READ(TWISTSW_TOP_GPIO_Port, TWISTSW_TOP_Pin) & 
                            PIN_READ(TWISTSW_BOT_GPIO_Port, TWISTSW_BOT_Pin);
        if (sw==0b11) switch_state[2] |= 1;
        if (sw==0b01) switch_state[2] |= 2;
        if (sw==0b10) switch_state[2] |= 4;
        if (sw==0b00) { while (1) {set_learn_green(flash++);} }
        if (switch_state[2]==7) set_learn_blue(0);

        sw = PIN_READ(WARPSW_TOP_GPIO_Port, WARPSW_TOP_Pin) & 
                            PIN_READ(WARPSW_BOT_GPIO_Port, WARPSW_BOT_Pin);
        if (sw==0b11) switch_state[3] |= 1;
        if (sw==0b01) switch_state[3] |= 2;
        if (sw==0b10) switch_state[3] |= 4;
        if (sw==0b00) { while (1) {set_freeze_green(flash++);} }
        if (switch_state[3]==7) set_freeze_blue(0);
    }

    wait_for_learn_released();
}

//one pot at a time: turn each pot down (red goes off), up (blue goes off), center (green goes off only when in center). 
//press button to select next pot
void test_pots(void) {
    adc_init_all();
}

