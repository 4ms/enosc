#include "codec.hh"
#include "spi_adc.hh"
#include "system.hh"
#include "debug.hh"
#include "ui.hh"
#include "polyptic_oscillator.hh"
#include "dynamic_data.hh"

Debug debug;

struct Main :
  System<kUiUpdateRate, Main>,
  Math,
  DynamicData,
  Codec<kSampleRate, kBlockSize, Main>,
  SpiAdc<2>,
  Ui<kUiUpdateRate, kBlockSize> {

  Main() {
    //Start audio processing
    Codec::Start();
    SpiAdc::Start();
    while(1) {
      Ui::Process();
      // TODO understand why this is crucial
      // just a "nop" is enough 
      __WFI();
    }
  }

  void SysTickCallback() {
    Ui::Update();
  }

  template<int block_size>
  void CodecCallback(Buffer<Frame, block_size>& out) {
    // debug.set(3, true);
    Ui::Poll();

    Ui::osc().Process(out);

    // debug.set(3, false);
  }
} _;
