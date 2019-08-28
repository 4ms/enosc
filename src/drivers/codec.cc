#include "codec.hh"

void (*codec_isr)();
void register_codec_isr(void f()) { codec_isr = f; }

// TODO: class Interrupt to directly redirect handler to the right function
extern "C" void DACSAI_SAI_TX_DMA_IRQHandler() {
  codec_isr();
}
