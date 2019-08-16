#pragma once

#include "hal.hh"
#include "dsp.hh"

enum SpiAdcInput {
  CV_PITCH,
  CV_ROOT,
  NUM_SPI_ADC_CHANNELS
};

enum max11666_channels {
	MAX11666_CHAN1 = 0x00FF,
	MAX11666_CHAN2 = 0xFF00
};

enum max11666Errors {
	MAX11666_NO_ERR = 0,
	MAX11666_SPI_INIT_ERR,
};

#define MAX11666_SPI_IRQHANDLER     SPI2_IRQHandler

void register_spi_adc_isr(void f());

//Oversample and average blocks of 2^oversampling_bitsize samples 
//Todo: Template with oversampling amount?
#define OVERSAMPLING_AMT_BITS   3
#define OVERSAMPLING_AMT (1<<OVERSAMPLING_AMT_BITS)
#define OVERSAMPLING_MASK (OVERSAMPLING_AMT-1)

struct SpiAdc : Nocopy {
	SpiAdc() {
    static_assert(OVERSAMPLING_AMT_BITS == 3, "FRAC bits of the return type of SpiAdc::get() must equal (OVERSAMPLING_AMT_BITS + ADC bits)");
    spiadc_instance_ = this;
    register_spi_adc_isr(SpiAdc::spiadc_ISR__IN_ITCM); //Todo: measure ITCM benefits

    err = MAX11666_NO_ERR;

    assign_pins();
    SPI_disable();
    SPI_GPIO_init();
    SPI_init();
    IRQ_init();
    SPI_enable();

    //Send one word to start
    cur_channel = MAX11666_CHAN1;
    spih.Instance->DR = cur_channel;
  }

  u4_12 get_last_raw(uint8_t chan) {
    return u4_12::of_repr(values[chan][os_idx[chan]]);
  }

  u1_15 get(uint8_t chan) {
    uint32_t avg = 0;
    for (int i=0; i<OVERSAMPLING_AMT; i++){
      avg += values[chan][i];
    }
    return u1_15::of_repr(avg);
  }

  static SpiAdc *spiadc_instance_;
  SPI_HandleTypeDef spih;
  uint16_t values[NUM_SPI_ADC_CHANNELS][OVERSAMPLING_AMT]={0};
  static uint32_t os_idx[NUM_SPI_ADC_CHANNELS];
  max11666_channels cur_channel;
  max11666Errors err;

private:
  typedef struct spiPin {
    uint16_t  pin;
    GPIO_TypeDef *gpio;
    uint32_t  gpio_clk;
    uint8_t   pin_source;
    uint8_t   af;
  } spiPin;

  uint32_t      SPI_clk;
  IRQn_Type     SPI_IRQn;
  spiPin        SCK;
  spiPin        MISO;
  spiPin        CHSEL;
  spiPin        CS;

  void assign_pins() {
    spih.Instance = SPI2;
    SPI_IRQn = SPI2_IRQn;

    SCK.pin = GPIO_PIN_13;
    SCK.gpio = GPIOB;
    SCK.af = GPIO_AF5_SPI2;

    MISO.pin = GPIO_PIN_14;
    MISO.gpio = GPIOB;
    MISO.af = GPIO_AF5_SPI2;

    CHSEL.pin = GPIO_PIN_15;
    CHSEL.gpio = GPIOB;
    CHSEL.af = GPIO_AF5_SPI2;

    CS.pin = GPIO_PIN_12;
    CS.gpio = GPIOB;
    CS.af = GPIO_AF5_SPI2;
  }

  void SPI_init() {
    spih.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    spih.Init.Direction         = SPI_DIRECTION_2LINES;
    spih.Init.CLKPhase          = SPI_PHASE_1EDGE;
    spih.Init.CLKPolarity       = SPI_POLARITY_LOW;
    spih.Init.DataSize          = SPI_DATASIZE_14BIT;
    spih.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    spih.Init.TIMode            = SPI_TIMODE_DISABLE;
    spih.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    spih.Init.CRCPolynomial     = 7;
    spih.Init.NSS               = SPI_NSS_HARD_OUTPUT;
    spih.Init.NSSPMode          = SPI_NSS_PULSE_ENABLE;
    spih.Init.Mode              = SPI_MODE_MASTER;

    if (HAL_SPI_Init(&spih) != HAL_OK)
      err = MAX11666_SPI_INIT_ERR;
  }

  void SPI_disable() {
    spih.Instance->CR1 &= ~SPI_CR1_SPE;
  }

  void SPI_enable() {
    spih.Instance->CR1 |= SPI_CR1_SPE;
  }

  static void spiadc_ISR__IN_ITCM();

  void IRQ_init()
  {
    HAL_NVIC_SetPriority(SPI_IRQn, 0, 1);
    HAL_NVIC_EnableIRQ(SPI_IRQn);

    // Enable the Rx buffer not empty interrupt
    __HAL_SPI_ENABLE_IT(&spih, SPI_IT_RXNE);

    __HAL_SPI_DISABLE_IT(&spih, SPI_IT_TXE);
  }

  void SPI_GPIO_init()
  {
    GPIO_InitTypeDef gpio;
    __HAL_RCC_GPIOB_CLK_ENABLE();

    #ifdef SPI1
      if (spih.Instance == SPI1)   
        __HAL_RCC_SPI1_CLK_ENABLE();
    #endif
    #ifdef SPI2
      if (spih.Instance == SPI2)   
        __HAL_RCC_SPI2_CLK_ENABLE();
    #endif
    #ifdef SPI3
      if (spih.Instance == SPI3)   
        __HAL_RCC_SPI3_CLK_ENABLE();
    #endif
    #ifdef SPI4
      if (spih.Instance == SPI4)   
        __HAL_RCC_SPI4_CLK_ENABLE();
    #endif
    #ifdef SPI5
      if (spih.Instance == SPI5)  
       __HAL_RCC_SPI5_CLK_ENABLE();
    #endif
    #ifdef SPI6
      if (spih.Instance == SPI6)   
        __HAL_RCC_SPI6_CLK_ENABLE();
    #endif

    gpio.Mode   = GPIO_MODE_AF_PP;
    gpio.Speed  = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Pull   = GPIO_PULLDOWN;

    gpio.Pin    = SCK.pin;
    gpio.Alternate  = SCK.af;
    HAL_GPIO_Init(SCK.gpio, &gpio);

    gpio.Pin    = CHSEL.pin;
    gpio.Alternate  = CHSEL.af;
    HAL_GPIO_Init(CHSEL.gpio, &gpio);

    gpio.Pin    = MISO.pin;
    gpio.Alternate  = MISO.af;
    HAL_GPIO_Init(MISO.gpio, &gpio);  

    gpio.Pin    = CS.pin;
    gpio.Alternate  = CS.af;
    HAL_GPIO_Init(CS.gpio, &gpio);
  }
};
