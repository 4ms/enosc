#include "spi_adc.hh"

void (*spi_adc_isr)();
void register_spi_adc_isr(void f()) { spi_adc_isr = f; }

extern "C" void MAX11666_SPI_IRQHANDLER(void) {
  spi_adc_isr();
}

//Todo: Fix scope issues so ISR can be part of SpiAdc struct
SpiAdc *SpiAdc::spiadc_instance_;

void spiadc_ISR() {
  uint32_t itflag = SpiAdc::spiadc_instance_->spih.Instance->SR;
  uint32_t itsource = SpiAdc::spiadc_instance_->spih.Instance->CR2;

  if ((itflag & SPI_FLAG_RXNE) && (itsource & SPI_IT_RXNE))
  { 
    if (SpiAdc::spiadc_instance_->cur_channel==MAX11666_CHAN1)
    {
      SpiAdc::spiadc_instance_->values[0] = SpiAdc::spiadc_instance_->spih.Instance->DR;
      SpiAdc::spiadc_instance_->cur_channel = MAX11666_CHAN2;
    } else {
      SpiAdc::spiadc_instance_->values[1] = SpiAdc::spiadc_instance_->spih.Instance->DR;
      SpiAdc::spiadc_instance_->cur_channel = MAX11666_CHAN1;
    }

    SpiAdc::spiadc_instance_->spih.Instance->DR = SpiAdc::spiadc_instance_->cur_channel;
  }
}
