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
    debug.set(3, true);
    ui_.Process(in, params_);

#ifdef BYPASS
    Frame *o_begin = out.begin();
    for(Frame i : in) {
      Frame &o = *o_begin;
      o = i;
      o_begin++;
    }
#else
#ifdef TEXTILE
    osc_.Process(params_, out);
#else
    osc_.Process(params_, out);
#endif
#endif
    debug.set(3, false);
  }
} _;
