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
  Codec<kSampleRate, kBlockSize, Main> {
  // TODO passer en dépendance?
  Ui<kBlockSize> ui_;

  Main() {
    //Start audio processing
    Codec<kSampleRate, kBlockSize, Main>::Start();
    while(1) {
      ui_.Process();
      // TODO understand why this is crucial
      // just a "nop" is enough 
      __WFI();
    }
  }

  template<int block_size>
  void codec_callback(Block<Frame, block_size> in, Block<Frame, block_size> out) {
    debug.set(3, true);
    ui_.Poll(in);

#ifdef BYPASS
    for(auto [i, o] : zip(in, out)) o = i;
#else
    ui_.osc().Process(out);
#endif
    debug.set(3, false);
  }
} _;
