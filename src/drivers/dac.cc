#include "dac.hh"

void (*dac_isr)();
void register_dac_isr(void f()) { dac_isr = f; }

// TODO: class Interrupt to directly redirect handler to the right function
extern "C" void DACSAI_SAI_TX_DMA_IRQHandler() {
  dac_isr();
}
