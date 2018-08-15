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
  Dac dac_ {I2S_FREQ_96000, this};
  Accelerometer accel_;
  Ui ui_;

  Accelerometer::AccelData d;

  FOscillators<20> fsine_;
  IOscillators<20> isine_;

  void Process(ShortFrame *out, int size) {
    debug.on(1);

    // accel_.ReadAccelData(&d);

    // // 42%
    // Float t[size];
    // for(int i=0; i<size; i++) {
    //   t[i] = fsine_.Process(0.004_f);
    //   out[i].l = s1_15(t[i]).repr();
    //   out[i].r = out[i].l;
    // }

    // 30.8%
    for(int i=0; i<size; i++) {
      out[i].l = isine_.Process(0.001_u0_32).repr();
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
