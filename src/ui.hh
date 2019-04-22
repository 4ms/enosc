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

enum Event {
  ButtonLearnPush,
  ButtonLearnRelease,
  ButtonLearnTimeout,
  ButtonFreezePush,
  ButtonFreezeRelease,
  ButtonFreezeTimeout,
  GateLearnOn,
  GateLearnOff,
  GateFreezeOn,
  GateFreezeOff,
  SwitchGridSwitched,
  SwitchModSwitched,
  SwitchTwistSwitched,
  SwitchWarpSwitched,
  KnobTurned,
};

struct ButtonsEventSource : EventSource<Event>, Buttons {
  void Poll(std::function<void(Event)> put) {
    Buttons::Debounce();
    if (Buttons::learn_.just_pressed()) put(ButtonLearnPush);
    else if (Buttons::learn_.just_released()) put(ButtonLearnRelease);
    if (Buttons::freeze_.just_pressed()) put(ButtonFreezePush);
    else if (Buttons::freeze_.just_released()) put(ButtonFreezeRelease);
  }
};

struct GatesEventSource : EventSource<Event>, Gates {
  void Poll(std::function<void(Event)> put) {
    Gates::Debounce();
    if (Gates::learn_.just_enabled()) put(GateLearnOn);
    else if (Gates::learn_.just_disabled()) put(GateLearnOff);
    if (Gates::freeze_.just_enabled()) put(GateFreezeOn);
    else if (Gates::freeze_.just_disabled()) put(GateFreezeOff);
  }
};

struct SwitchesEventSource : EventSource<Event>, Switches {
  void Poll(std::function<void(Event)> put) {
    Switches::Debounce();
    if (Switches::grid_.just_switched()) put(SwitchGridSwitched);
    if (Switches::mod_.just_switched()) put(SwitchModSwitched);
    if (Switches::twist_.just_switched()) put(SwitchTwistSwitched);
    if (Switches::warp_.just_switched()) put(SwitchWarpSwitched);
  }
};

template<int block_size>
class Ui : public EventHandler<Ui<block_size>, Event> {
  using Base = EventHandler<Ui, Event>;
  friend Base;
  Parameters params_;
  Switches switches_;
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
  Control<block_size> control_ {osc_};

  static constexpr int kLongPressTime = 0.5f * kSampleRate / block_size;

  LedManager<Leds::Learn> learn_led_;
  LedManager<Leds::Freeze> freeze_led_;

  typename Base::DelayedEventSource learn_timeout_;
  typename Base::DelayedEventSource freeze_timeout_;
  ButtonsEventSource buttons_;
  GatesEventSource gates_;

  EventSource<Event>* sources_[4] = {
    &buttons_, &gates_, &learn_timeout_, &freeze_timeout_
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

  void onButtonLearnLongPress() {
  }

  void onButtonLearnPress() {
    if (mode == Mode::CALIBRATION) {
      mode = Mode::NORMAL;
    } else if (mode == Mode::NORMAL) {
        set_learn(!osc_.learn_enabled());
    }
  }

  void onButtonFreezePress() {
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
  }

  void onButtonFreezeLongPress() {
  }

  void Handle(typename Base::EventStack stack) {
    switch(stack.get(0)) {
    case ButtonLearnPush: {
      learn_timeout_.trigger_after(kLongPressTime, ButtonLearnTimeout);
    } break;
    case ButtonLearnRelease: {
      switch(stack.get(1)) {
      case ButtonLearnPush: onButtonLearnPress(); break;
      default: break;
      }
    } break;
    case ButtonLearnTimeout: {
      switch(stack.get(1)) {
      case ButtonLearnPush: onButtonLearnLongPress(); break;
      default: break;
      }
    } break;
    case ButtonFreezePush: {
      freeze_timeout_.trigger_after(kLongPressTime, ButtonFreezeTimeout);
    } break;
    case ButtonFreezeRelease: {
      switch(stack.get(1)) {
      case ButtonFreezePush: onButtonFreezePress(); break;
      default: break;
      }
    } break;
    case ButtonFreezeTimeout: {
      switch(stack.get(1)) {
      case ButtonFreezePush: onButtonFreezeLongPress(); break;
      default: break;
      }
    } break;
    case GateLearnOff: {
    } break;
    case GateFreezeOn: {
    } break;
    case GateFreezeOff: {
    } break;
    case SwitchGridSwitched: {
    } break;
    case SwitchModSwitched: {
    } break;
    case SwitchTwistSwitched: {
    } break;
    case SwitchWarpSwitched: {
    } break;
    case KnobTurned: {
    } break;
    }
  }

public:
  Ui() {}

  PolypticOscillator<block_size>& osc() { return osc_; }

  void Poll(Block<Frame, block_size> codec_in) {
    Base::Poll();
    control_.Process(codec_in, params_);
  }

  void Process() {
    Base::Process();

    // Switches
    Switches::State tw = switches_.twist_.get();
    params_.twist.mode =
      tw == Switches::UP ? FEEDBACK :
      tw == Switches::CENTER ? PULSAR : DECIMATE;
    Switches::State wa = switches_.warp_.get();
    params_.warp.mode =
      wa == Switches::UP ? FOLD :
      wa == Switches::CENTER ? CHEBY : CRUSH;
    Switches::State gr = switches_.grid_.get();
    params_.grid.mode =
      gr == Switches::UP ? CHORD :
      gr == Switches::CENTER ? HARM : JUST;
    Switches::State mo = switches_.mod_.get();
    params_.modulation.mode =
      mo == Switches::UP ? ONE :
      mo == Switches::CENTER ? TWO : THREE;

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
