#include "hal.hh"
#include "codec.hh"
#include "system.hh"
#include "debug.hh"
#include "ui.hh"
#include "polyptic_oscillator.hh"
#include "textile_oscillator.hh"

// #define BYPASS

struct Main :
  Codec<kSampleRate, kBlockSize, Main> {
  // TODO passer en dépendance?
  System sys_;
  Ui<kBlockSize> ui_;

  Main() {
    //Start audio processing
    Codec<kSampleRate, kBlockSize, Main>::Start();
    while(1) { }
  }

  template<int block_size>
  void codec_callback(Block<Frame, block_size> in, Block<Frame, block_size> out) {
    debug.set(3, true);
    ui_.Process(in);

#ifdef BYPASS
    for(auto x : zip(in, out)) {
      get<1>(x) = get<0>(x);
    }
#else
    ui_.osc().Process(out);
#endif
    debug.set(3, false);
  }
} _;
