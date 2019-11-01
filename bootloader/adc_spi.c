#include "adc_spi.h"
#include "stm32f7xx_ll_bus.h"
#include "stm32f7xx_ll_spi.h"
#include "stm32f7xx_ll_gpio.h"

enum max11666Errors adcspi_err = MAX11666_NO_ERR;

void ADCSPI_GPIO_init(void);
void ADCSPI_SPI_init(void);
void ADCSPI_IRQ_init(void);

void setup_ADCSPI_pin(uint32_t pin);

uint16_t spiadc_val;
uint8_t spiadc_reading_chan = 0;

uint16_t get_adc_spi_value(void) { return spiadc_val>>2; }

void set_adc_spi_channel(uint8_t chan) {
    spiadc_reading_chan = chan ? 1 : 0;
}

void init_adc_spi(void) {
    LL_SPI_Disable(ADCSPIx);


    ADCSPI_GPIO_init();
    ADCSPI_SPI_init();
    ADCSPI_IRQ_init();

    LL_SPI_Enable(ADCSPIx);

    //Start
    LL_SPI_TransmitData16(ADCSPIx, MAX11666_CONTINUE_READING_CH1);
}

void ADCSPI_SPI_init(void)
{
    LL_SPI_SetBaudRatePrescaler(ADCSPIx, LL_SPI_BAUDRATEPRESCALER_DIV8);
    LL_SPI_SetTransferDirection(ADCSPIx, LL_SPI_FULL_DUPLEX);
    LL_SPI_SetClockPhase(ADCSPIx, LL_SPI_PHASE_1EDGE);
    LL_SPI_SetClockPolarity(ADCSPIx, LL_SPI_POLARITY_LOW);
    LL_SPI_SetTransferBitOrder(ADCSPIx, LL_SPI_MSB_FIRST); //reset value
    LL_SPI_SetDataWidth(ADCSPIx, LL_SPI_DATAWIDTH_16BIT);
    LL_SPI_SetNSSMode(ADCSPIx, LL_SPI_NSS_HARD_OUTPUT);
    LL_SPI_SetRxFIFOThreshold(ADCSPIx, LL_SPI_RX_FIFO_EMPTY);
    LL_SPI_SetMode(ADCSPIx, LL_SPI_MODE_MASTER);
    LL_SPI_SetStandard(ADCSPIx, LL_SPI_PROTOCOL_MOTOROLA);// reset value
    LL_SPI_EnableNSSPulseMgt(ADCSPIx);
    LL_SPI_DisableCRC(ADCSPIx);
    LL_SPI_SetCRCPolynomial(ADCSPIx, 7);
}


void ADCSPI_IRQ_init(void)
{
    NVIC_SetPriority(ADCSPI_IRQn, 0b0100);
    NVIC_EnableIRQ(ADCSPI_IRQn);

    // Enable the Rx buffer not empty and error interrupts
    LL_SPI_EnableIT_RXNE(ADCSPIx);
    LL_SPI_DisableIT_ERR(ADCSPIx);
    LL_SPI_DisableIT_TXE(ADCSPIx);
}

void setup_ADCSPI_pin(uint32_t pin)
{
    LL_GPIO_SetPinOutputType(ADCSPI_GPIO, pin, LL_GPIO_OUTPUT_PUSHPULL);
    LL_GPIO_SetPinMode(ADCSPI_GPIO, pin, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinSpeed(ADCSPI_GPIO, pin, LL_GPIO_SPEED_FREQ_VERY_HIGH);
    LL_GPIO_SetAFPin_8_15(ADCSPI_GPIO, pin, ADCSPI_GPIO_AF);
    LL_GPIO_SetPinPull(ADCSPI_GPIO, pin, LL_GPIO_PULL_DOWN);
}
void ADCSPI_GPIO_init(void)
{
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_SPI2);

    setup_ADCSPI_pin(ADCSPI_SCK_pin);
    setup_ADCSPI_pin(ADCSPI_MISO_pin);
    setup_ADCSPI_pin(ADCSPI_CHSEL_pin);
    setup_ADCSPI_pin(ADCSPI_CS_pin);
}

void ADCSPI_IRQHandler(void)
{
    if (LL_SPI_IsActiveFlag_RXNE(ADCSPIx))
    {
        spiadc_val = LL_SPI_ReceiveData16(ADCSPIx);

        LL_SPI_TransmitData16(ADCSPIx, spiadc_reading_chan ? MAX11666_CONTINUE_READING_CH2 : MAX11666_CONTINUE_READING_CH1);
    }
    else if (LL_SPI_IsActiveFlag_TXE(ADCSPIx))
    {
        LL_SPI_TransmitData16(ADCSPIx, spiadc_reading_chan ? MAX11666_CONTINUE_READING_CH2 : MAX11666_CONTINUE_READING_CH1);
    }
    else if (LL_SPI_IsActiveFlag_OVR(ADCSPIx))
    {
        //Error
        adcspi_err = MAX11666_SPI_OVR_ERR;
    }
}
