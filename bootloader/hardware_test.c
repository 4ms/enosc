#include "hardware_test.h"
#include "gpio_pins.h"
#include "adc.h"
#include "lib/stm32f7xx_ll_gpio.h"
#include "bl_utils.h"
#include "buttons.h"
#include "leds.h"
#include "dac_sai.h"
#include "saw_osc.h"
#include "adc_spi.h"
#include "qspi_flash.h"

extern volatile uint32_t systmr;

void test_leds(void);
void test_switches(void);
void test_builtin_adc(void);
void test_dac(void);
void test_extadc(void);
void test_gates(void);
void test_QSPI(void);


void do_hardware_test(void) {
    test_leds();
    test_switches();
    test_dac();
    test_builtin_adc();
    test_extadc();
    test_QSPI();
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

        if (switch_state[0] + switch_state[1] + switch_state[2] + switch_state[3] == 7+7+7+7) break;
    }

    wait_for_learn_released();
}

//bit-bangs reg init for dac
//init SAI
//send leaning triangle
void test_dac(void)
{
    SET_LEARN_YELLOW();
    SET_FREEZE_RED();

    init_dac();

    set_dac_callback(saw_out);
    set_saw_freqs(10000, -20000, 20000, -20000);
    set_saw_ranges(-6710885, 6710885, -6710885, 6710885);

    start_dac();

    SET_LEARN_YELLOW();
    SET_FREEZE_GREEN();

    wait_for_learn_pressed();
    wait_for_learn_released();

    SET_LEARN_OFF();
    SET_FREEZE_OFF();

    //Setup for testing ADCs next
    set_saw_freqs(1000, -2000, 2000, -2000);
    set_saw_ranges(-2000000, 6710885, -3733335, 3933335);
}

struct AdcCheck {
    const uint16_t center_val;
    const uint16_t center_width;
    const uint16_t min_val;
    const uint16_t max_val;
    const uint32_t center_check_rate; 
    uint16_t cur_val;
    uint32_t status;
};
//returns 1 if adc is fully ranged checked
uint8_t check_adc(struct AdcCheck *adc_check)
{
    if (adc_check->cur_val < adc_check->min_val) {
        LEARN_RED(OFF);
        adc_check->status &= ~(0b10UL);
    }
    if (adc_check->cur_val > adc_check->max_val) {
        LEARN_BLUE(OFF);
        adc_check->status &= ~(0b01UL);
    }
    if (adc_check->cur_val>(adc_check->center_val - adc_check->center_width) \
        && adc_check->cur_val<(adc_check->center_val + adc_check->center_width)) {
        LEARN_GREEN(OFF);
        adc_check->status -= adc_check->center_check_rate; //count down
    }
    else {
        adc_check->status |= ~(0b11UL); //reset counter
        LEARN_GREEN(ON);
    }
    if ((adc_check->status & 0xFFFF0003)==0)
        return 1;
    else
        return 0;
}

//one adc at a time: turn each pot or send CV into each jack
//-5V/CCW (red goes off), 5V/CW (blue goes off), 0V/center (green goes off only when in center). 
//press button to select next adc
void test_builtin_adc(void) {
    SET_FREEZE_GREEN();
    SET_LEARN_RED();

    adc_init_all();

    SET_FREEZE_OFF();
    SET_LEARN_WHITE();

    struct AdcCheck adc_check = {
        .center_val = 2048,
        .center_width = 50,
        .min_val = 10,
        .max_val = 4000,
        .center_check_rate = (1UL<<16)
    };
    for (uint32_t cur_adc=0; cur_adc<(NUM_ADCS-1); cur_adc++) {
        adc_check.status = 0xFFFFFFFF;
        while (!learn_pressed()) {
            adc_check.cur_val = read_adc(cur_adc<NUM_POT_ADC1 ? ADC1 : ADC3, cur_adc);
            if (check_adc(&adc_check)) 
                break;
        }
        FREEZE_GREEN(ON);
        delay(1500);
        FREEZE_GREEN(OFF);
        wait_for_learn_released();
    }
}

void test_gates(void) {
    SET_FREEZE_OFF();

    LL_GPIO_SetPinMode(FREEZE_JACK_GPIO_Port, FREEZE_JACK_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(FREEZE_JACK_GPIO_Port, FREEZE_JACK_Pin, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinMode(LEARN_JACK_GPIO_Port, LEARN_JACK_Pin, LL_GPIO_MODE_INPUT);
    LL_GPIO_SetPinPull(LEARN_JACK_GPIO_Port, LEARN_JACK_Pin, LL_GPIO_PULL_UP);

    for (uint32_t i=0; i<2; i++) {
        SET_LEARN_WHITE();
        uint32_t status = 0b111;
        while (!learn_pressed()) {
            uint32_t gate =  i ? PIN_READ(FREEZE_JACK_GPIO_Port, FREEZE_JACK_Pin) : 
                                 PIN_READ(LEARN_JACK_GPIO_Port, LEARN_JACK_Pin);
            if (!gate && (status & 0b010)) {
                LEARN_GREEN(OFF);
                status &= ~(0b001UL);
            }
            if (gate) {
                LEARN_RED(OFF);
                status &= ~(0b010UL);
            }   
            if (!gate && !(status & 0b010)) {
                LEARN_BLUE(OFF);
                status &= ~(0b100UL);
            }
            if (status==0) break;
        }
        FREEZE_GREEN(ON);
        delay(1500);
        FREEZE_GREEN(OFF);
        wait_for_learn_released();
    }
}


//checks SPI communication
//reads value from pitch/root jacks: 
//red goes off when 6V read, blue goes off after that and -2V read. Green goes off after that when 0V read
void test_extadc(void) {
    SET_LEARN_RED();
    SET_FREEZE_RED();

    init_adc_spi();

    delay(1500);

    SET_LEARN_WHITE();
    SET_FREEZE_OFF();

    struct AdcCheck adc_check = {
        .center_val = 2907,
        .center_width = 200,
        .min_val = 10,
        .max_val = 4000,
        .center_check_rate = (1UL<<15)
    };
    for (uint8_t chan=0; chan<2; chan++) {
        adc_check.status = 0xFFFFFFFF;
        while (!learn_pressed()) {
            adc_check.cur_val = get_adc_spi_value();
            if (check_adc(&adc_check))
                break;
        }
        FREEZE_GREEN(ON);
        set_adc_spi_channel(1);
        delay(1500);
        wait_for_learn_released();
        FREEZE_GREEN(OFF);
        SET_LEARN_WHITE();
    }
    SET_LEARN_OFF();
}



uint8_t test_encode_num(uint32_t num) {return (num*7) + (num>>7);}

// Tests one sector
// Returns true if passed, false if failed
uint32_t test_qspi_sector(uint8_t sector_num)
{
    uint32_t i;
    uint8_t test_buffer[QSPI_SECTOR_SIZE];
    uint32_t test_addr = sector_num * QSPI_SECTOR_SIZE;

    for (i=0; i<QSPI_SECTOR_SIZE; i++)
        test_buffer[i] = (test_encode_num(i) + sector_num) & 0xFF;
    
    LEARN_BLUE(ON);
    //Benchmark: ~38ms/sector
    if (!QSPI_erase(SECTOR_ERASE_CMD, test_addr))
        return 0;

    LEARN_BLUE(OFF);
    LEARN_RED(ON);
    for (i=0; i<(QSPI_SECTOR_SIZE/QSPI_PAGE_SIZE); i++)
    {
        FREEZE_RED((i&1) ? ON : OFF);
        FREEZE_GREEN((i&2) ? ON : OFF);
        FREEZE_BLUE((i&4) ? ON : OFF);
        //Benchmark: ~380us/page
        if (!QSPI_write_page( &(test_buffer[i*QSPI_PAGE_SIZE]), test_addr+i*QSPI_PAGE_SIZE, QSPI_PAGE_SIZE))
            return 0;
    }
    LEARN_RED(OFF);
    SET_FREEZE_OFF();

    for (i=0; i<QSPI_SECTOR_SIZE; i++)
        test_buffer[i] = 0;

    LEARN_GREEN(ON);
    //Benchmark: ~680-850us/sector
    if (!QSPI_read(test_buffer, test_addr, QSPI_SECTOR_SIZE))
        return 0;
    LEARN_GREEN(OFF);

    for (i=0; i<(QSPI_SECTOR_SIZE-1); i++) {
        if (test_buffer[i] != ((test_encode_num(i) + sector_num) & 0xFF))
            return 0;
    }

    return 1;
}

//internal read/write/compare test: warning: erases entire chip
void test_QSPI(void) {
    SET_LEARN_CYAN();
    SET_FREEZE_RED();

    if (!QSPI_init())
    {
        FREEZE_RED(ON);
        delay(1000);
        FREEZE_RED(OFF);
        delay(1000);
    }

    delay(1500);

    SET_LEARN_RED();
    SET_FREEZE_OFF();

    uint8_t sector;
    for (sector=0; sector<QSPI_NUM_SECTORS; sector++) {
        if (!test_qspi_sector(sector))
        {
            while (1) {
                SET_LEARN_RED();
                SET_FREEZE_RED();
                delay(1000);
                LEARN_RED(OFF);
                FREEZE_RED(OFF);
                delay(1000);
            }
            break;
        }
    }

    SET_LEARN_OFF();
    SET_FREEZE_OFF();
}



