#include "hardware_test.h"
#include "gpio_pins.h"
#include "adc.h"
#include "lib/stm32f7xx_ll_gpio.h"
#include "bl_utils.h"
#include "buttons.h"
#include "leds.h"
#include "dac_sai.h"

extern volatile uint32_t systmr;

void test_leds(void);
void test_switches(void);
void test_pots(void);
void test_dac(void);

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
    test_dac();
    test_pots();

    while (1) {;}
}



//press buttons to turn off each led color: red green blue
void test_leds(void) {
    wait_for_learn_released();
    wait_for_freeze_released();

    SET_FREEZE_WHITE();
    SET_LEARN_WHITE();

    wait_for_learn_pressed();
    LEARN_RED(OFF);
    wait_for_learn_released();
    wait_for_learn_pressed();
    LEARN_GREEN(OFF);
    wait_for_learn_released();
    wait_for_learn_pressed();
    LEARN_BLUE(OFF);
    wait_for_learn_released();

    wait_for_freeze_pressed();
    FREEZE_RED(OFF);
    wait_for_freeze_released();
    wait_for_freeze_pressed();
    FREEZE_GREEN(OFF);
    wait_for_freeze_released();
    wait_for_freeze_pressed();
    FREEZE_BLUE(OFF);
    wait_for_freeze_released();
}

//flip switches to turn leds off: each switch must be found in 3 positions before turning an element off
//only red/blue are used (four switches, four led elements)
void test_switches(void) {
    uint32_t switch_state[4] = {0};
    // GPIO_InitTypeDef gpio;

    //GPIOs A (freeze LED), C (learn LED) are already enabled
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOBEN);
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOEEN);
    SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIODEN);

    LL_GPIO_SetPinMode(CROSSFMSW_TOP_GPIO_Port, CROSSFMSW_TOP_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(CROSSFMSW_TOP_GPIO_Port, CROSSFMSW_TOP_Pin, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinMode(CROSSFMSW_TOP_GPIO_Port, CROSSFMSW_BOT_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(CROSSFMSW_TOP_GPIO_Port, CROSSFMSW_BOT_Pin, LL_GPIO_PULL_UP);

    LL_GPIO_SetPinMode(SCALESW_TOP_GPIO_Port, SCALESW_TOP_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(SCALESW_TOP_GPIO_Port, SCALESW_TOP_Pin, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinMode(SCALESW_TOP_GPIO_Port, SCALESW_BOT_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(SCALESW_TOP_GPIO_Port, SCALESW_BOT_Pin, LL_GPIO_PULL_UP);

    LL_GPIO_SetPinMode(TWISTSW_TOP_GPIO_Port, TWISTSW_TOP_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(TWISTSW_TOP_GPIO_Port, TWISTSW_TOP_Pin, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinMode(TWISTSW_TOP_GPIO_Port, TWISTSW_BOT_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(TWISTSW_TOP_GPIO_Port, TWISTSW_BOT_Pin, LL_GPIO_PULL_UP);

    LL_GPIO_SetPinMode(WARPSW_TOP_GPIO_Port, WARPSW_TOP_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(WARPSW_TOP_GPIO_Port, WARPSW_TOP_Pin, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinMode(WARPSW_BOT_GPIO_Port, WARPSW_BOT_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(WARPSW_BOT_GPIO_Port, WARPSW_BOT_Pin, LL_GPIO_PULL_UP);

    FREEZE_RED(ON);
    FREEZE_BLUE(ON);
    LEARN_RED(ON);
    LEARN_BLUE(ON);

    uint32_t sw;
    uint8_t flash=0;
    while (!learn_pressed()){
        sw = PIN_READ(CROSSFMSW_TOP_GPIO_Port, CROSSFMSW_TOP_Pin) | 
                            (PIN_READ(CROSSFMSW_BOT_GPIO_Port, CROSSFMSW_BOT_Pin) << 1);
        if (sw==0b11) switch_state[0] |= 1;
        if (sw==0b01) switch_state[0] |= 2;
        if (sw==0b10) switch_state[0] |= 4;
        if (sw==0b00) { LEARN_GREEN(ON); while (1) {;} }
        if (switch_state[0]==7) FREEZE_RED(OFF); //switch passes

        sw = PIN_READ(SCALESW_TOP_GPIO_Port, SCALESW_TOP_Pin) | 
                            (PIN_READ(SCALESW_BOT_GPIO_Port, SCALESW_BOT_Pin) << 1);
        if (sw==0b11) switch_state[1] |= 1;
        if (sw==0b01) switch_state[1] |= 2;
        if (sw==0b10) switch_state[1] |= 4;
        if (sw==0b00) { LEARN_GREEN(ON); while (1) {;} }
        if (switch_state[1]==7) LEARN_RED(OFF);

        sw = PIN_READ(TWISTSW_TOP_GPIO_Port, TWISTSW_TOP_Pin) | 
                            (PIN_READ(TWISTSW_BOT_GPIO_Port, TWISTSW_BOT_Pin) << 1);
        if (sw==0b11) switch_state[2] |= 1;
        if (sw==0b01) switch_state[2] |= 2;
        if (sw==0b10) switch_state[2] |= 4;
        if (sw==0b00) { while (1) {LEARN_GREEN(ON);LEARN_GREEN(OFF);} }
        if (switch_state[2]==7) LEARN_BLUE(OFF);

        sw = PIN_READ(WARPSW_TOP_GPIO_Port, WARPSW_TOP_Pin) | 
                            (PIN_READ(WARPSW_BOT_GPIO_Port, WARPSW_BOT_Pin) << 1);
        if (sw==0b11) switch_state[3] |= 1;
        if (sw==0b01) switch_state[3] |= 2;
        if (sw==0b10) switch_state[3] |= 4;
        if (sw==0b00) { while (1) {FREEZE_GREEN(ON);FREEZE_GREEN(OFF);} }
        if (switch_state[3]==7) FREEZE_BLUE(OFF);
    }

    wait_for_learn_released();
}

void triangle_out(int32_t *dst) {
    static int32_t tri;
    uint32_t i;
    for (i=0; i<DAC_FRAMES_PER_BLOCK; i++) {
        *dst++ = tri++;
        *dst++ = tri++;
    }

};

//bit-bangs reg init for dac
//init SAI
//send leaning triangle
void test_dac(void)
{
    init_dac();

    set_dac_callback(triangle_out);

    start_dac();

}



//one adc at a time: turn each pot or send CV into each jack
//-5V/CCW (red goes off), 5V/CW (blue goes off), 0V/center (green goes off only when in center). 
//press button to select next adc
void test_pots(void) {
    adc_init_all();
    uint16_t adcval;

    SET_FREEZE_OFF();
    SET_LEARN_WHITE();

    for (uint32_t cur_adc=0; cur_adc<(NUM_ADCS-1); cur_adc++) {
        while (!learn_pressed()) {
            if (cur_adc<NUM_POT_ADC1)
                adcval = read_adc(ADC1, cur_adc);
            else
                adcval = read_adc(ADC3, cur_adc);

            if (adcval<10) LEARN_RED(OFF);
            if (adcval>4000) LEARN_BLUE(OFF);
            if (adcval>2000 && adcval<2100) LEARN_GREEN(OFF);
            else LEARN_GREEN(ON);
        }

        wait_for_learn_released();
        SET_LEARN_WHITE();
    }
}

