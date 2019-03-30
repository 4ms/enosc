#include "hal.hh"
#include "codec.hh"
#include "system.hh"
#include "debug.hh"
#include "ui.hh"
#include "polyptic_oscillator.hh"
#include "textile_oscillator.hh"

// #define BYPASS

struct Main : Nocopy {
  System sys_;
  Ui<kBlockSize> ui_;

  Codec codec_{kSampleRate, [this](auto in, auto out) {this->Process(in, out);}};

  Main() {
    //Start audio processing
    codec_.Start();
    while(1) { }
  }

  template<int size>
  void Process(Block<Frame, size> in, Block<Frame, size> out) {
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
