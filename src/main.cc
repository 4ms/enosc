#include "hal.hh"
#include "codec.hh"
#include "system.hh"
#include "debug.hh"
#include "ui.hh"
#include "polyptic_oscillator.hh"
#include "dynamic_data.hh"

// #define BYPASS

struct Main :
  System,
  DynamicData,
  Codec<kSampleRate, kBlockSize, Main>,
  Ui<kBlockSize> {

  Main() {
    //Start audio processing
    Codec::Start();
    while(1) {
      Ui::Process();
      // TODO understand why this is crucial
      // just a "nop" is enough 
      __WFI();
    }
  }

  template<int block_size>
  void CodecCallback(Block<Frame, block_size> in, Block<Frame, block_size> out) {
    debug.set(3, true);
    Ui::Poll(in);

#ifdef BYPASS
    for(auto [i, o] : zip(in, out)) o = i;
#else
    Ui::osc().Process(out);
#endif
    debug.set(3, false);
  }
} _;
