#include "drivers/debug_pins.hh"
#include "drivers/dac.hh"
#include "drivers/system.hh"
#include "drivers/accelerometer.hh"

#include "ui.hh"
#include "oscillator.hh"

struct Main :
  System::SysTickCallback,
  Dac::ProcessCallback {

  System system_ {this};
  Dac dac_ {I2S_FREQ_48000, this};
  Accelerometer accel_;
  Ui ui_;

  Accelerometer::AccelData d;

  void Process(ShortFrame *out, int size) {
    debug.on(1);

    // accel_.ReadAccelData(&d);

    // // 52%
    // Float t[size];
    // static FOscillators<40> sine_;
    // for(int i=0; i<size; i++) {
    //   t[i] = sine_.Process(0.005_f);
    //   out[i].l = s1_15(t[i]).repr();
    //   out[i].r = out[i].l;
    // }

    // // 52%
    // Float t[size];
    // static IFOscillators<40> sine_;
    // for(int i=0; i<size; i++) {
    //   t[i] = sine_.Process(0.005_u0_32);
    //   out[i].l = s1_15(t[i]).repr();
    //   out[i].r = out[i].l;
    // }

    // 31%
    static IOscillators<40> sine_;
    for(int i=0; i<size; i++) {
      out[i].l = sine_.Process(0.001_u0_32).repr();
      out[i].r = out[i].l;
    }

    debug.off(1);
  }

  void onSysTick() {
    ui_.Poll();
    ui_.Display();
  }

  Main() {
    dac_.set_volume(230);
    dac_.Start();
    while(1) { }
  }
} _;
