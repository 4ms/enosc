#include "buttons.hh"
#include "gates.hh"
#include "switches.hh"
#include "leds.hh"
#include "control.hh"
#include "polyptic_oscillator.hh"

enum Button {BUTTON_LEARN, BUTTON_FREEZE};
enum Gate {GATE_LEARN, GATE_FREEZE};

template<class T>
struct LedManager : Leds::ILed<T> {
  using L = Leds::ILed<T>;
  void flash(Color c) {
    flash_color = c;
    flash_phase = u0_16::max_val;
  }
  void set_background(Color b) { background = b; }
  void Update() {
    Color c = background.blend(flash_color, u0_8::narrow(flash_phase));
    Leds::ILed<T>::set(c);
    if (flash_phase > flash_time) flash_phase -= flash_time;
  }
private:
  Color background = Colors::black;
  Color flash_color = Colors::white;
  u0_16 flash_time = 0.0014_u0_16;
  u0_16 flash_phase = 0._u0_16;
};

class Ui {
  Buttons buttons_;
  Gates gates_;
  Switches switches_;
  Leds leds_;
  PolypticOscillator osc_ {
    [this](bool success) {
      // on new note
      learn_led_.flash(success ? Colors::white : Colors::black);
    },
    [this](bool success) {
      // on exit of Learn
      if(success) learn_led_.flash(Colors::magenta);
    }
  };
  Control control_ {osc_};

  LedManager<Leds::Learn> learn_led_;
  LedManager<Leds::Freeze> freeze_led_;

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

  void button_pressed(Button b) {
    switch(b) {
    case BUTTON_LEARN: {
      set_learn(!osc_.learn_enabled());
    } break;
    case BUTTON_FREEZE: {
    } break;
    }
  }

  void button_released(Button b) {
    switch(b) {
    case BUTTON_LEARN: {
    } break;
    case BUTTON_FREEZE: {
    } break;
    }
  }

  void gate_enabled(Gate g) {
    switch(g) {
    case GATE_LEARN:
      set_learn(true);
      break;
    case GATE_FREEZE:
      break;
    }
  }

  void gate_disabled(Gate g) {
    switch(g) {
    case GATE_LEARN:
      set_learn(false);
      break;
    case GATE_FREEZE:
      break;
    }
  }

public:
  PolypticOscillator& osc() { return osc_; }

  void Process(Block<Frame> codec_in, Parameters& params) {

    // Controls
    control_.Process(codec_in, params);

    // Buttons
    buttons_.Debounce();

    if (buttons_.learn_.just_pressed())
      button_pressed(BUTTON_LEARN);
    else if (buttons_.learn_.just_released())
      button_released(BUTTON_LEARN);
    if (buttons_.freeze_.just_pressed())
      button_pressed(BUTTON_FREEZE);
    else if (buttons_.freeze_.just_released())
      button_released(BUTTON_FREEZE);

    //Gate jacks
    gates_.Debounce();

    if (gates_.learn_.just_enabled())
      gate_enabled(GATE_LEARN);
    else if (gates_.learn_.just_disabled())
      gate_disabled(GATE_LEARN);
    if (gates_.freeze_.just_enabled())
      gate_enabled(GATE_FREEZE);
    else if (gates_.freeze_.just_disabled())
      gate_disabled(GATE_FREEZE);

    //Switches
    params.twist.mode = static_cast<TwistMode>(switches_.twist_.get());
    params.warp.mode = static_cast<WarpMode>(switches_.warp_.get());
    params.grid.mode = static_cast<GridMode>(switches_.grid_.get());
    // TODO temp
    params.modulation.mode = static_cast<ModulationMode>(switches_.mod_.get());

    // LEDs
    switch (mode) {
    case NORMAL_MODE:
      bool b = osc_.learn_enabled();
      learn_led_.set_background(b ? Colors::dark_red : Colors::black);
      break;
    }

    learn_led_.Update();
    freeze_led_.Update();
  }
};
