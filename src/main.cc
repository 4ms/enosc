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
  Ui ui_ {osc_};
#ifdef TEXTILE
  TextileOscillator osc_;
#else
  PolypticOscillator osc_;
#endif
  Parameters params_;

  Codec codec_{kSampleRate,
               [this](DoubleBlock<Frame, Frame> inout) {
                 this->Process(inout);
               }};

  Main() {
    //Start audio processing
    codec_.Start();
    while(1) { }
  }

  void Process(DoubleBlock<Frame, Frame> inout) {
    debug.set(3, true);
    ui_.Process(inout.first(), params_);

#ifdef BYPASS
    // TODO double block
    Frame *o_begin = out.begin();
    for(Frame i : in) {
      Frame &o = *o_begin;
      o = i;
      o_begin++;
    }
#else
    osc_.Process(params_, inout.second());
#endif
    debug.set(3, false);
  }
} _;
