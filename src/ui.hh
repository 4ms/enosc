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

  void flash(Color c, u0_16 flash_freq = 0.0014_u0_16) {
    flash_color_ = c;
    flash_phase_ = u0_16::max_val;
    flash_freq_ = flash_freq;
  }

  void set_background(Color c) { background_color_ = c; }

  void set_glow(Color c, u0_32 freq) {
    glow_color_ = c;
    osc_.set_frequency(freq);
  }

  void reset_glow(u0_32 phase = 0._u0_32) {
    osc_.set_frequency(0._u0_32);
    osc_.set_phase(phase);
  }

  void Update() {
    Color c = background_color_;
    c = c.blend(glow_color_, u0_8::narrow(osc_.Process()));
    c = c.blend(flash_color_, u0_8::narrow(flash_phase_));
    Leds::ILed<T>::set(c);
    if (flash_phase_ > flash_freq_) flash_phase_ -= flash_freq_;
  }

private:
  TriangleOscillator osc_;
  Color background_color_ = Colors::black;
  Color flash_color_ = Colors::white;
  Color glow_color_ = Colors::red;
  u0_16 flash_freq_ = 0.0014_u0_16;
  u0_16 flash_phase_ = 0._u0_16;
};

struct ButtonsEventSource : EventSource<Event>, Buttons {
  void Poll(std::function<void(Event)> const& put) {
    Buttons::Debounce();
    if (Buttons::learn_.just_pushed()) put({ButtonPush, BUTTON_LEARN});
    else if (Buttons::learn_.just_released()) put({ButtonRelease, BUTTON_LEARN});
    if (Buttons::freeze_.just_pushed()) put({ButtonPush, BUTTON_FREEZE});
    else if (Buttons::freeze_.just_released()) put({ButtonRelease, BUTTON_FREEZE});
  }
};

struct GatesEventSource : EventSource<Event>, private Gates {
  void Poll(std::function<void(Event)> const& put) {
    Gates::Debounce();
    if (Gates::learn_.just_enabled()) put({GateOn, GATE_LEARN});
    else if (Gates::learn_.just_disabled()) put({GateOff, GATE_LEARN});
    if (Gates::freeze_.just_enabled()) put({GateOn, GATE_FREEZE});
    else if (Gates::freeze_.just_disabled()) put({GateOff, GATE_FREEZE});
  }
};

struct SwitchesEventSource : EventSource<Event>, private Switches {
  void Poll(std::function<void(Event)> const& put) {
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

  static constexpr int kLongPressTime = 1.0f * kSampleRate / block_size;

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
    SHIFT,
    CALIBRATION_OFFSET,
    CALIBRATION_SLOPE,
  } mode_ = Mode::NORMAL;

  uint8_t selected_osc_ = 0;
  uint8_t active_catchups_ = 0;

  void set_mode(Mode mode) {
    switch(mode) {
    case Mode::NORMAL: {
      learn_led_.reset_glow();
      learn_led_.set_background(Colors::black);
      freeze_led_.reset_glow();
      freeze_led_.set_background(Colors::black);
    } break;
    case Mode::SHIFT: {
      freeze_led_.set_background(Colors::grey);
    } break;
    case Mode::CALIBRATION_OFFSET: {
      learn_led_.set_glow(Colors::red, 0.0004_u0_32);
      freeze_led_.set_glow(Colors::red, 0.0004_u0_32);
    } break;
    case Mode::CALIBRATION_SLOPE: {
      learn_led_.set_glow(Colors::dark_magenta, 0.0008_u0_32);
      freeze_led_.set_glow(Colors::dark_magenta, 0.0008_u0_32);
    } break;
    }
    mode_ = mode;
  }

  void set_learn(bool b) {
    if (b) {
      learn_led_.set_background(Colors::dark_red);
      osc_.enable_learn();
      control_.hold_pitch_cv();
    } else {
      learn_led_.set_background(Colors::black);
      osc_.disable_learn();
      control_.release_pitch_cv();
    }
  }

  void onButtonLongPress(Button b) {
  }

  void onButtonPress(Button b) {
    switch(b) {
    case BUTTON_LEARN: {
      if (mode_ == Mode::CALIBRATION_OFFSET) {
        if (control_.CalibrateOffset())
          set_mode(Mode::CALIBRATION_SLOPE);
        else // calibration failure
          set_mode(Mode::NORMAL);
      } else if (mode_ == Mode::CALIBRATION_SLOPE) {
        if (control_.CalibrateSlope()) {
          learn_led_.flash(Colors::white);
          freeze_led_.flash(Colors::white);
        }
        set_mode(Mode::NORMAL);
      } else if (mode_ == Mode::NORMAL) {
        set_learn(!osc_.learn_enabled());
      }
    } break;
    case BUTTON_FREEZE: {
      if (mode_ == Mode::CALIBRATION_OFFSET ||
          mode_ == Mode::CALIBRATION_SLOPE) {
        set_mode(Mode::NORMAL);
      } else if (mode_ == Mode::NORMAL) {
        u0_8 freeze_level = u0_8(f(selected_osc_) / f(params_.numOsc));
        freeze_led_.set_background(Colors::black.blend(Colors::blue, freeze_level));
        osc_.freeze(selected_osc_);
        selected_osc_++;
        if (selected_osc_ > params_.numOsc) {
          selected_osc_ = 0;
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
    if (mode_ == Mode::SHIFT) {
      freeze_led_.flash(Colors::white);
      params_.stereo_mode =
        st == Switches::UP ? ALTERNATE :
        st == Switches::MID ? SPLIT : LOWER_REST;
    } else {
      params_.twist.mode =
        st == Switches::UP ? FEEDBACK :
        st == Switches::MID ? PULSAR : DECIMATE;
    }
  }

  void onSwitchWarpSwitched(Switches::State st) {
    params_.warp.mode =
      st == Switches::UP ? FOLD :
      st == Switches::MID ? CHEBY : CRUSH;
  }

  void onPotMoved(AdcInput input) {
    if (mode_ == Mode::SHIFT) {
      if (input == ROOT_POT)
        control_.root_pot_alternate_function();
    }
  }

  void onNewNote() { osc_.new_note(control_.pitch_cv()); }
  void onShiftEnter() { set_mode(Mode::SHIFT); }
  void onShiftExit() {
    control_.all_main_function();
    set_mode(Mode::NORMAL);
  }
  void onGridChanged() { learn_led_.flash(Colors::white); }
  void onNumOscChanged() { freeze_led_.flash(Colors::white); }

  void onStartCatchup() {
    active_catchups_++;
    freeze_led_.set_glow(Colors::grey, 0.0003_u0_32 * active_catchups_);
  }
  void onEndOfCatchup() {
    active_catchups_--;
    if (active_catchups_ == 0)
      freeze_led_.reset_glow();
  }

  void Handle(typename Base::EventStack stack) {
    Event& e1 = stack.get(0);
    Event& e2 = stack.get(1);

    if (mode_ == Mode::SHIFT &&
        e1.type == ButtonRelease &&
        e1.data == BUTTON_FREEZE) {
      onShiftExit();
      return;
    }

    if (e2.type == ButtonPush &&
        e2.data == BUTTON_FREEZE &&
        e1.type != ButtonRelease &&
        e1.type != ButtonTimeout) {
      onShiftEnter();
    }

    switch(e1.type) {
    case ButtonPush: {
      button_timeouts_[e1.data].trigger_after(kLongPressTime, {ButtonTimeout, e1.data});
    } break;
    case ButtonRelease: {
      if (e2.type == ButtonPush && e1.data == e2.data) {
        onButtonPress(static_cast<Button>(e1.data));
      }
    } break;
    case ButtonTimeout: {
      if (e2.type == ButtonPush && e1.data == e2.data) {
        onButtonLongPress(static_cast<Button>(e1.data));
      }
    } break;
    case GateOn: {
      if (e1.data == GATE_LEARN)
        new_note_delay_.trigger_after(50, {NewNote, 0});
    } break;
    case GateOff: {
    } break;
    case SwitchGridSwitched: onSwitchGridSwitched(static_cast<Switches::State>(e1.data)); break;
    case SwitchModSwitched: onSwitchModSwitched(static_cast<Switches::State>(e1.data)); break;
    case SwitchTwistSwitched: onSwitchTwistSwitched(static_cast<Switches::State>(e1.data)); break;
    case SwitchWarpSwitched: onSwitchWarpSwitched(static_cast<Switches::State>(e1.data)); break;
    case PotMoved: onPotMoved(static_cast<AdcInput>(e1.data)); break;
    case NewNote: onNewNote(); break;
    case GridChanged: onGridChanged(); break;
    case NumOscChanged: onNumOscChanged(); break;
    case StartCatchup: onStartCatchup(); break;
    case EndOfCatchup: onEndOfCatchup(); break;
    }
  }

public:
  Ui() {
    // Initialize switches to their current positions
    onSwitchGridSwitched(switches_.get_grid());
    onSwitchModSwitched(switches_.get_mod());
    onSwitchTwistSwitched(switches_.get_twist());
    onSwitchWarpSwitched(switches_.get_warp());

    // Enter calibration if Learn is pushed
    if (buttons_.learn_.pushed()) {
      set_mode(Mode::CALIBRATION_OFFSET);
    } else {
      set_mode(Mode::NORMAL);
    }
  }

  PolypticOscillator<block_size>& osc() { return osc_; }

  void Poll(Block<Frame, block_size> codec_in) {
    control_.ProcessCodecInput(codec_in);
    Base::Poll();
  }

  void Process() {
    Base::Process();

    learn_led_.Update();
    freeze_led_.Update();
  }
};
