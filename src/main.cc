#include "hal.hh"
#include "codec.hh"
#include "system.hh"
#include "debug.hh"
#include "ui.hh"
#include "polyptic_oscillator.hh"
#include "textile_oscillator.hh"

// #define TEXTILE
// #define BYPASS

struct Main : Nocopy {
  System sys_;
  Ui ui_;
#ifdef TEXTILE
  TextileOscillator osc_;
#else
  PolypticOscillator osc_;
#endif
  Parameters params_;

  Codec codec_{kSampleRate,
               [this](Block<Frame> in, Block<Frame> out) {
                 this->Process(in, out);
               }};

  Main() {
    //Start audio processing
    codec_.Start();

    while(1) { }
  }

  void Process(Block<Frame> in, Block<Frame> out) {
    ui_.Process(params_);

#ifdef BYPASS
      while(size--) {
        *out = *in;
        out++;
        in++;
      }
      return;
#endif

    debug.set(3, true);
#ifdef TEXTILE
    osc_.Process(params_, out);
#else
    osc_.Process(params_, out);
#endif
    debug.set(3, false);
  }
} _;
