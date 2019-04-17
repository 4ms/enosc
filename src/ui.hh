#pragma once

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
    L::set(c);
    if (flash_phase > flash_time) flash_phase -= flash_time;
  }
private:
  Color background = Colors::black;
  Color flash_color = Colors::white;
  u0_16 flash_time = 0.0014_u0_16;
  u0_16 flash_phase = 0._u0_16;
};

struct DelayedEvent {
  void trigger_in(int d, std::function<void()> a) { delay_ = d; action_ = a; }
  void Process() { if (delay_-- == 0) { action_(); } }
  DelayedEvent() {}
private:
  std::function<void()> action_ = [](){};
  int delay_ = -1;
};

template<int size>
class Ui {
  Parameters params_;
  Buttons buttons_;
  Gates gates_;
  Switches switches_;
  Leds leds_;
  PolypticOscillator<size> osc_ {
    params_,
    [this](bool success) {
      // on new note
      learn_led_.flash(success ? Colors::white : Colors::black);
    },
    [this](bool success) {
      // on exit of Learn
      if(success) learn_led_.flash(Colors::magenta);
    }
  };
  Control<size> control_ {osc_};

  LedManager<Leds::Learn> learn_led_;
  LedManager<Leds::Freeze> freeze_led_;

  enum UiMode {
    NORMAL_MODE,
  } mode = NORMAL_MODE;
  DelayedEvent new_note_event_;


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
    } break;
    case BUTTON_FREEZE: {
    } break;
    }
  }

  void button_released(Button b) {
    switch(b) {
    case BUTTON_LEARN: {
      set_learn(!osc_.learn_enabled());
    } break;
    case BUTTON_FREEZE: {
      osc_.freeze_selected_osc();
      params_.selected_osc++;
      if (params_.selected_osc == params_.numOsc+1) {
        params_.selected_osc = 0;
        osc_.unfreeze_all();
      }
    } break;
    }
  }

  void gate_enabled(Gate g) {
    switch(g) {
    case GATE_LEARN:
      new_note_event_.trigger_in(40, [this] { osc_.new_note(control_.pitch_cv()); });
      break;
    case GATE_FREEZE:
      break;
    }
  }

  void gate_disabled(Gate g) {
    switch(g) {
    case GATE_LEARN:
      break;
    case GATE_FREEZE:
      break;
    }
  }

public:
  PolypticOscillator<size>& osc() { return osc_; }

  Ui() {
    leds_.learn_.set(Colors::red);
    leds_.freeze_.set(Colors::red);
  }

  void Process(Block<Frame, size> codec_in) {

    // Delays
    new_note_event_.Process();

    // Controls
    control_.Process(codec_in, params_);

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

    // Gate jacks
    gates_.Debounce();

    if (gates_.learn_.just_enabled())
      gate_enabled(GATE_LEARN);
    else if (gates_.learn_.just_disabled())
      gate_disabled(GATE_LEARN);
    if (gates_.freeze_.just_enabled())
      gate_enabled(GATE_FREEZE);
    else if (gates_.freeze_.just_disabled())
      gate_disabled(GATE_FREEZE);

    // Switches
    params_.twist.mode = static_cast<TwistMode>(switches_.twist_.get());
    params_.warp.mode = static_cast<WarpMode>(switches_.warp_.get());
    params_.grid.mode = static_cast<GridMode>(switches_.grid_.get());
    params_.modulation.mode = static_cast<ModulationMode>(switches_.mod_.get());

    // LEDs
    switch (mode) {
    case NORMAL_MODE:
      bool b = osc_.learn_enabled();
      learn_led_.set_background(b ? Colors::dark_red : Colors::black);
      u0_8 freeze_level = u0_8(f(params_.selected_osc) / f(params_.numOsc+1));
      freeze_led_.set_background(Colors::black.blend(Colors::blue, freeze_level));
      break;
    }

    learn_led_.Update();
    freeze_led_.Update();
  }
};
