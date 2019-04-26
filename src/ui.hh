#pragma once

#include <variant>
#include "buttons.hh"
#include "gates.hh"
#include "switches.hh"
#include "leds.hh"
#include "control.hh"
#include "polyptic_oscillator.hh"
#include "event_handler.hh"

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

struct ButtonsEventSource : EventSource<Event>, private Buttons {
  void Poll(std::function<void(Event)> put) {
    Buttons::Debounce();
    if (Buttons::learn_.just_pushed()) put({ButtonPush, BUTTON_LEARN});
    else if (Buttons::learn_.just_released()) put({ButtonRelease, BUTTON_LEARN});
    if (Buttons::freeze_.just_pushed()) put({ButtonPush, BUTTON_FREEZE});
    else if (Buttons::freeze_.just_released()) put({ButtonRelease, BUTTON_FREEZE});
  }
};

struct GatesEventSource : EventSource<Event>, private Gates {
  void Poll(std::function<void(Event)> put) {
    Gates::Debounce();
    if (Gates::learn_.just_enabled()) put({GateOn, GATE_LEARN});
    else if (Gates::learn_.just_disabled()) put({GateOff, GATE_LEARN});
    if (Gates::freeze_.just_enabled()) put({GateOn, GATE_FREEZE});
    else if (Gates::freeze_.just_disabled()) put({GateOff, GATE_FREEZE});
  }
};

struct SwitchesEventSource : EventSource<Event>, private Switches {
  void Poll(std::function<void(Event)> put) {
    Switches::Debounce();

    if (Switches::grid_.just_switched_up()) put({SwitchGridSwitched, UP});
    if (Switches::grid_.just_switched_mid()) put({SwitchGridSwitched, MID});
    if (Switches::grid_.just_switched_down()) put({SwitchGridSwitched, DOWN});

    if (Switches::mod_.just_switched_up()) put({SwitchModSwitched, UP});
    if (Switches::mod_.just_switched_mid()) put({SwitchModSwitched, MID});
    if (Switches::mod_.just_switched_down()) put({SwitchModSwitched, DOWN});

    if (Switches::twist_.just_switched_up()) put({SwitchTwistSwitched, UP});
    if (Switches::twist_.just_switched_mid()) put({SwitchTwistSwitched, MID});
    if (Switches::twist_.just_switched_down()) put({SwitchTwistSwitched, DOWN});

    if (Switches::warp_.just_switched_up()) put({SwitchWarpSwitched, UP});
    if (Switches::warp_.just_switched_mid()) put({SwitchWarpSwitched, MID});
    if (Switches::warp_.just_switched_down()) put({SwitchWarpSwitched, DOWN});
  }

  Switches::State get_grid() { return Switches::grid_.get(); }
  Switches::State get_mod() { return Switches::mod_.get(); }
  Switches::State get_twist() { return Switches::twist_.get(); }
  Switches::State get_warp() { return Switches::warp_.get(); }
};

template<int block_size>
class Ui : public EventHandler<Ui<block_size>, Event> {
  using Base = EventHandler<Ui, Event>;
  friend Base;

  Parameters params_;
  Leds leds_;
  PolypticOscillator<block_size> osc_ {
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

  static constexpr int kLongPressTime = 0.5f * kSampleRate / block_size;

  LedManager<Leds::Learn> learn_led_;
  LedManager<Leds::Freeze> freeze_led_;

  typename Base::DelayedEventSource button_timeouts_[2];
  typename Base::DelayedEventSource new_note_delay_;
  ButtonsEventSource buttons_;
  GatesEventSource gates_;
  SwitchesEventSource switches_;
  Control<block_size> control_ {osc_, params_};

  EventSource<Event>* sources_[7] = {
    &buttons_, &gates_, &switches_,
    &button_timeouts_[0], &button_timeouts_[1],
    &control_, &new_note_delay_
  };

  enum class Mode {
    NORMAL,
    CALIBRATION,
  } mode = Mode::NORMAL;

  void set_learn(bool b) {
    if (b) {
      osc_.enable_learn();
      control_.hold_pitch_cv();
    } else {
      osc_.disable_learn();
      control_.release_pitch_cv();
    }
  }

  void onButtonLongPress(Button b) {
  }

  void onButtonPress(Button b) {
    switch(b) {
    case BUTTON_LEARN: {
        if (mode == Mode::CALIBRATION) {
          mode = Mode::NORMAL;
        } else if (mode == Mode::NORMAL) {
          set_learn(!osc_.learn_enabled());
        }
      } break;
    case BUTTON_FREEZE: {
        if (mode == Mode::CALIBRATION) {
          mode = Mode::NORMAL;
        } else if (mode == Mode::NORMAL) {
          osc_.freeze_selected_osc();
          params_.selected_osc++;
          if (params_.selected_osc == params_.numOsc+1) {
            params_.selected_osc = 0;
            osc_.unfreeze_all();
          }
        }
      } break;
    }
  }

  void onSwitchGridSwitched(Switches::State st) {
    params_.grid.mode =
      st == Switches::UP ? CHORD :
      st == Switches::MID ? HARM : JUST;
  }

  void onSwitchModSwitched(Switches::State st) {
    params_.modulation.mode =
      st == Switches::UP ? ONE :
      st == Switches::MID ? TWO : THREE;
  }

  void onSwitchTwistSwitched(Switches::State st) {
    params_.twist.mode =
      st == Switches::UP ? FEEDBACK :
      st == Switches::MID ? PULSAR : DECIMATE;
  }

  void onSwitchWarpSwitched(Switches::State st) {
    params_.warp.mode =
      st == Switches::UP ? FOLD :
      st == Switches::MID ? CHEBY : CRUSH;
  }

  void onNewNote() {
    osc_.new_note(control_.pitch_cv());
  }

  void Handle(typename Base::EventStack stack) {
    Event& e = stack.get(0);
    switch(e.type) {
    case ButtonPush: {
      button_timeouts_[e.data].trigger_after(kLongPressTime, {ButtonTimeout, e.data});
    } break;
    case ButtonRelease: {
      Event& e2 = stack.get(1);
      if (e2.type == ButtonPush && e.data == e2.data) {
        onButtonPress(static_cast<Button>(e.data));
      }
    } break;
    case ButtonTimeout: {
      Event e2 = stack.get(1);
      if (e2.type == ButtonPush && e.data == e2.data) {
        onButtonLongPress(static_cast<Button>(e.data));
      }
    } break;
    case GateOn: {
      if (e.data == GATE_LEARN)
        new_note_delay_.trigger_after(50, {NewNote, 0});
    } break;
    case GateOff: {
    } break;
    case SwitchGridSwitched: onSwitchGridSwitched(static_cast<Switches::State>(e.data)); break;
    case SwitchModSwitched: onSwitchModSwitched(static_cast<Switches::State>(e.data)); break;
    case SwitchTwistSwitched: onSwitchTwistSwitched(static_cast<Switches::State>(e.data)); break;
    case SwitchWarpSwitched: onSwitchWarpSwitched(static_cast<Switches::State>(e.data)); break;
    case KnobTurned: {
    } break;
    case NewNote: {
      onNewNote();
    } break;
    }
  }

public:
  Ui() {
    // Initialize switches to their current positions
    onSwitchGridSwitched(switches_.get_grid());
    onSwitchModSwitched(switches_.get_mod());
    onSwitchTwistSwitched(switches_.get_twist());
    onSwitchWarpSwitched(switches_.get_warp());
  }

  PolypticOscillator<block_size>& osc() { return osc_; }

  void Poll(Block<Frame, block_size> codec_in) {
    control_.ProcessCodecInput(codec_in);
    Base::Poll();
  }

  void Process() {
    Base::Process();

    // LEDs
    switch (mode) {
    case Mode::NORMAL: {
      bool b = osc_.learn_enabled();
      learn_led_.set_background(b ? Colors::dark_red : Colors::black);
      u0_8 freeze_level = u0_8(f(params_.selected_osc) / f(params_.numOsc+1));
      freeze_led_.set_background(Colors::black.blend(Colors::blue, freeze_level));
    } break;
    case Mode::CALIBRATION: {
      learn_led_.set_background(Colors::red);
      freeze_led_.set_background(Colors::red);
    } break;
    }

    learn_led_.Update();
    freeze_led_.Update();
  }
};
