#include "buttons.hh"
#include "gates.hh"
#include "switches.hh"
#include "leds.hh"
#include "control.hh"
#include "polyptic_oscillator.hh"

enum Button {LEARN, FREEZE};

class Ui {
  Buttons buttons_;
  Gates gates_;
  Switches switches_;
  Leds leds_;
  PolypticOscillator &osc_;
  Control control_ {osc_};

  // UI state variables
  bool freeze_jack, learn_jack;
  bool learn_but, freeze_but;
  Switches::State mod_sw, grid_sw, twist_sw, warp_sw;

  enum UiMode {
    NORMAL_MODE,
  } mode = NORMAL_MODE;

  void set_learn(bool b) {
    if (b) {
      osc_.enable_learn();
      control_.hold_pitch_cv();
    } else {
      osc_.disable_learn();
      control_.release_pitch_cv();
    }
  }

public:
  Ui(PolypticOscillator &osc) : osc_(osc) {}

  void button_pressed(Button b) {
    switch(b) {
    case LEARN: {
      set_learn(!osc_.learn_enabled());
    } break;
    case FREEZE: {
    } break;
    }
  }

  void button_released(Button b) {
    switch(b) {
    case LEARN: {
    } break;
    case FREEZE: {
    } break;
    }
  }

  void Process(Block<Frame> codec_in, Parameters& params) {

    // Controls
    bool pitch_cv_changed = false;
    control_.Process(codec_in, params, pitch_cv_changed);
    // TODO: remove after use
    if (pitch_cv_changed) {
      leds_.freeze_.set(WHITE);
    } else {
      leds_.freeze_.set(BLACK);
    }

    // Buttons
    buttons_.Debounce();

    if (buttons_.learn_.just_pressed())
      button_pressed(LEARN);
    else if (buttons_.learn_.just_released())
      button_released(LEARN);
    if (buttons_.freeze_.just_pressed())
      button_pressed(FREEZE);
    else if (buttons_.freeze_.just_released())
      button_released(FREEZE);

    //Gate jacks
    freeze_jack = gates_.freeze_.get();
    learn_jack = gates_.learn_.get();

    //Switches
    params.twist.mode = static_cast<TwistMode>(switches_.twist_.get());
    params.warp.mode = static_cast<WarpMode>(switches_.warp_.get());
    params.grid.mode = static_cast<GridMode>(switches_.grid_.get());
    // TODO temp
    params.stereo_mode = static_cast<StereoMode>(switches_.mod_.get());

    // LEDs
    switch (mode) {
    case NORMAL_MODE:
      bool b = osc_.learn_enabled();
      leds_.learn_.set(b ? RED : BLACK);
      // leds_.freeze_.set(BLACK);
      break;
    }
  }
};
