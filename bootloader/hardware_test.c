#include "hardware_test.h"

//press buttons to change led color: red green blue off
void test_leds(void);

//flip switches to turn leds off: each switch must be found in 3 positions before turning an element off
//only red/blue are used (four switches, four led elements)
void test_switches(void);

//one pot at a time: turn each pot down (red goes off), up (blue goes off), center (green goes off only when in center). 
//press button to select next pot
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

}
