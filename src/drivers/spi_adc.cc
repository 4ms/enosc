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
bool SpiAdc::skip=false;
uint8_t SpiAdc::read_sequence_idx;
uint8_t SpiAdc::cur_chan=0;

//Info: SPI ISR takes ~116ns to execute, and runs 15 times between every call to Ui.Poll()
//Each execution is 2.7us after the previous, until the Dac's ISR takes control for ~120us
void SpiAdc::spiadc_ISR__IN_ITCM_() {
  uint32_t itflag = SpiAdc::spiadc_instance_->spih.Instance->SR;
  uint32_t itsource = SpiAdc::spiadc_instance_->spih.Instance->CR2;

  if ((itflag & SPI_FLAG_RXNE) && (itsource & SPI_IT_RXNE))
  {
    uint16_t adc_val = SpiAdc::spiadc_instance_->spih.Instance->DR;

    if (skip) {
      skip = false;
    } else {
      SpiAdc::spiadc_instance_->values[cur_chan][os_idx[cur_chan]] = u1_15::of_repr(adc_val);
      os_idx[cur_chan]++;
      os_idx[cur_chan] &= OVERSAMPLING_MASK;

      SpiAdc::spiadc_instance_->read_sequence_idx++;

      if ((SpiAdc::spiadc_instance_->read_sequence_idx & OVERSAMPLING_MASK)==0) {
        skip = true;
        cur_chan = !cur_chan;
      }
    }

    //Todo: Make sure the chip accepts 0x00FF twice in a row
    SpiAdc::spiadc_instance_->spih.Instance->DR = cur_chan ? MAX11666_CHAN2 : MAX11666_CHAN1;
  }
}

/*
MASK=0111
AMT = 1000

0000 0
0001 1
0010 2
...  3-5
0110 6
0111 7

1000 skip
1000 0
*/