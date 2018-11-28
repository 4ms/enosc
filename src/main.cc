#include "hal.hh"
#include "codec.hh"
#include "system.hh"
#include "debug.hh"
#include "ui.hh"
#include "polyptic_oscillator.hh"

struct Main : Nocopy {
  System sys_;
  Ui ui_;
  PolypticOscillator osc_;
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
    debug.set(3, true);
    ui_.Process(params_);

    if (ui_.bypass) {
      while(size--) {
        *out = *in;
        out++;
        in++;
      }
      return;
    }

    osc_.Process(params_, in, out, size);
    debug.set(3, false);
  }
} _;
