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
               [this](Frame* in, Frame *out, int size) {
                 this->Process(in, out, size);
               }};

  Main() {
    //Start audio processing
    codec_.Start();

    while(1) { }
  }

  void Process(Frame *in, Frame *out, int size) {
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
    osc_.Process(params_, Block<Frame>{out, size});
#else
    osc_.Process(params_, Block<Frame> {out, size});
#endif
    debug.set(3, false);
  }
} _;
