#include "spi_adc.hh"
#include "debug.hh"
// extern Debug debug;

SpiAdc *SpiAdc::spiadc_instance_;
uint32_t SpiAdc::os_idx[NUM_SPI_ADC_CHANNELS]={0};
uint8_t SpiAdc::cur_chan;

extern "C" void MAX11666_SPI_IRQHANDLER(void) {
  uint32_t itflag = SpiAdc::spiadc_instance_->spih.Instance->SR;
  uint32_t itsource = SpiAdc::spiadc_instance_->spih.Instance->CR2;

  if ((itflag & SPI_FLAG_RXNE) && (itsource & SPI_IT_RXNE))
  { 
    // debug.set(SpiAdc::cur_chan, true);
    uint16_t adc_val = SpiAdc::spiadc_instance_->spih.Instance->DR;
    adc_val >>= 2;
    if (SpiAdc::os_idx[SpiAdc::cur_chan] < kOversamplingAmount)
      SpiAdc::spiadc_instance_->values[SpiAdc::cur_chan][SpiAdc::os_idx[SpiAdc::cur_chan]] = u2_14::of_repr(adc_val);

    if (SpiAdc::os_idx[SpiAdc::cur_chan]) {
      SpiAdc::os_idx[SpiAdc::cur_chan]--;
      SpiAdc::spiadc_instance_->spih.Instance->DR = SpiAdc::cur_chan ? MAX11666_CONTINUE_READING_CH2 : MAX11666_CONTINUE_READING_CH1;
   }
   // debug.set(SpiAdc::cur_chan, false);

  }
}

