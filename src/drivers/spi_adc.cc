#include "spi_adc.hh"
#include "debug.hh"
extern Debug debug;

void (*spi_adc_isr)();
void register_spi_adc_isr(void f()) { spi_adc_isr = f; }

extern "C" void MAX11666_SPI_IRQHANDLER(void) {
  spi_adc_isr();
}

SpiAdc *SpiAdc::spiadc_instance_;
uint32_t SpiAdc::os_idx[NUM_SPI_ADC_CHANNELS]={0};

//Info: SPI ISR takes ~116ns to execute, and runs 15 times between every call to Ui.Poll()
//Each execution is 2.7us after the previous, until the Dac's ISR takes control for ~120us
void SpiAdc::spiadc_ISR__IN_ITCM_() {
  uint32_t itflag = SpiAdc::spiadc_instance_->spih.Instance->SR;
  uint32_t itsource = SpiAdc::spiadc_instance_->spih.Instance->CR2;

  if ((itflag & SPI_FLAG_RXNE) && (itsource & SPI_IT_RXNE))
  { 
    uint8_t chan = (SpiAdc::spiadc_instance_->cur_channel == MAX11666_CHAN2);
    SpiAdc::spiadc_instance_->values[chan][os_idx[chan]] =
      u1_15::of_repr(SpiAdc::spiadc_instance_->spih.Instance->DR);
    os_idx[chan]++;
    os_idx[chan] &= OVERSAMPLING_MASK;
    SpiAdc::spiadc_instance_->cur_channel = chan ? MAX11666_CHAN1 : MAX11666_CHAN2;
    SpiAdc::spiadc_instance_->spih.Instance->DR = SpiAdc::spiadc_instance_->cur_channel;
  }
}
