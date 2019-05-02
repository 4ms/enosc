#pragma once

#include <variant>
#include "buttons.hh"
#include "gates.hh"
#include "switches.hh"
#include "leds.hh"
#include "control.hh"
#include "polyptic_oscillator.hh"
#include "event_handler.hh"

template<int update_rate, class T>
struct LedManager : Leds::ILed<T> {

  // flash_freq in Hz; max = update_rate
  void flash(Color c, f flash_freq = 10_f) {
    flash_color_ = c;
    flash_phase_ = u0_16::max_val;
    flash_freq_ = u0_16(flash_freq / f(update_rate));
  }

  void set_background(Color c) { background_color_ = c; }

  // freq in secs
  void set_glow(Color c, f freq = 1_f) {
    glow_color_ = c;
    osc_.set_frequency(u0_32(freq / f(update_rate)));
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

template<int update_rate, int block_size>
class Ui : public EventHandler<Ui<update_rate, block_size>, Event> {
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

  static constexpr int kProcessRate = kSampleRate / block_size;
  static constexpr int kLongPressTime = 1.0f * kProcessRate; // sec
  static constexpr int kNewNoteDelayTime = 0.01f * kProcessRate; // sec

  LedManager<update_rate, Leds::Learn> learn_led_;
  LedManager<update_rate, Leds::Freeze> freeze_led_;

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

  enum Mode {
    NORMAL,
    SHIFT,
    LEARN,
    MANUAL_LEARN,
    CALIBRATION_OFFSET,
    CALIBRATION_SLOPE,
  } mode_ = NORMAL;

  uint8_t selected_osc_ = 0;
  uint8_t active_catchups_ = 0;

  void Handle(typename Base::EventStack stack) {
    Event& e1 = stack.get(0);
    Event& e2 = stack.get(1);

    switch(mode_) {

    case NORMAL: {

      switch(e1.type) {
      case ButtonRelease: {
        if (e2.type == ButtonPush &&
            e1.data == e2.data) {
          // Button press
          if (e1.data == BUTTON_LEARN) {
            mode_ = LEARN;
            learn_led_.set_background(Colors::dark_red);
            osc_.enable_learn();
            control_.hold_pitch_cv();
          } else {
            // press freeze
          }
        }
      } break;
      case ButtonPush: {
        button_timeouts_[e1.data].trigger_after(kLongPressTime, {ButtonTimeout, e1.data});
        if (e1.data == BUTTON_FREEZE) {
          mode_ = SHIFT;
        }
      } break;
      case SwitchGridSwitched: {
        params_.grid.mode =
          e1.data == Switches::UP ? CHORD :
          e1.data == Switches::MID ? HARM : JUST;
      } break;
      case SwitchModSwitched: {
        params_.modulation.mode =
          e1.data == Switches::UP ? ONE :
          e1.data == Switches::MID ? TWO : THREE;
      } break;
      case SwitchTwistSwitched: {
        params_.twist.mode =
          e1.data == Switches::UP ? FEEDBACK :
          e1.data == Switches::MID ? PULSAR : DECIMATE;
      } break;
      case SwitchWarpSwitched: {
        params_.warp.mode =
          e1.data == Switches::UP ? FOLD :
          e1.data == Switches::MID ? CHEBY : CRUSH;
      } break;
      case StartCatchup: {
        active_catchups_++;
        freeze_led_.set_glow(Colors::grey, 2_f * f(active_catchups_));
      } break;
      case EndOfCatchup: {
        active_catchups_--;
        freeze_led_.set_glow(Colors::grey, 2_f * f(active_catchups_));
      } break;
      case GridChanged: {
        learn_led_.flash(Colors::white);
      } break;
      }
    } break;

    case LEARN: {
      switch(e1.type) {
      case GateOn: {
        if (e1.data == GATE_LEARN) {
          new_note_delay_.trigger_after(kNewNoteDelayTime, {NewNote, 0});
        }
      } break;
      case NewNote: {
        osc_.new_note(control_.pitch_cv());
      } break;
      case PotMoved: {
        if (e1.data == ROOT_POT &&
            e2.type == ButtonPush &&
            e2.data == BUTTON_LEARN) {
          mode_ = MANUAL_LEARN;
          learn_led_.set_glow(Colors::red, 3_f);
          osc_.enable_pre_listen();
        }
      } break;
      case ButtonRelease: {
        if (e1.data == BUTTON_LEARN &&
            e2.type == ButtonPush &&
            e2.data == BUTTON_LEARN) {
          mode_ = NORMAL;
          osc_.disable_learn();
          control_.release_pitch_cv();
          freeze_led_.set_background(Colors::black);
        }
      } break;
      }
    } break;

    case SHIFT: {
      switch(e1.type) {
      case SwitchWarpSwitched: {
        freeze_led_.flash(Colors::white);
        freeze_led_.set_background(Colors::grey);
        params_.crossfade_factor =
          e1.data == Switches::UP ? Crossfade::linear :
          e1.data == Switches::MID ? Crossfade::mid : Crossfade::steep;
      } break;
      case SwitchTwistSwitched: {
        freeze_led_.flash(Colors::white);
        freeze_led_.set_background(Colors::grey);
        params_.stereo_mode =
          e1.data == Switches::UP ? ALTERNATE :
          e1.data == Switches::MID ? SPLIT : LOWER_REST;
      } break;
      case PotMoved: {
        if (e1.data == GRID_POT) {
          freeze_led_.set_background(Colors::grey);
          control_.grid_pot_alternate_function();
        }
      }
      case ButtonRelease: {
        if (e1.data == BUTTON_FREEZE) {
          if (e2.type == ButtonPush &&
              e2.data == BUTTON_FREEZE) {
            // Freeze press
            u0_8 freeze_level = u0_8(f(selected_osc_) / f(params_.numOsc));
            freeze_led_.set_background(Colors::black.blend(Colors::blue, freeze_level));
            osc_.freeze(selected_osc_);
            selected_osc_++;
            if (selected_osc_ > params_.numOsc) {
              selected_osc_ = 0;
              osc_.unfreeze_all();
            }
          } else {
            mode_ = NORMAL;
            control_.all_main_function();
            freeze_led_.set_background(Colors::black);
          }
        }
      } break;
      case NumOscChanged: {
        freeze_led_.flash(Colors::white);
      } break;
      }
    } break;

    case MANUAL_LEARN: {

      if (e1.type == ButtonRelease && e1.data == BUTTON_LEARN) {
        learn_led_.reset_glow();
        osc_.new_note(control_.pitch_cv());
      }

    } break;

    case CALIBRATION_OFFSET: {
      if (e1.type == ButtonRelease &&
          e1.data == BUTTON_LEARN &&
          e2.type == ButtonPush &&
          e2.data == BUTTON_LEARN) {
        if (control_.CalibrateOffset()) {
          mode_ = CALIBRATION_SLOPE;
          learn_led_.set_glow(Colors::dark_magenta, 1_f);
          freeze_led_.set_glow(Colors::dark_magenta, 1_f);
        } else {
          learn_led_.reset_glow();
          freeze_led_.reset_glow();
          mode_ = NORMAL;       // calibration failure
        }
      } else if (e1.type == ButtonPush &&
                 e1.data == BUTTON_FREEZE) {
        learn_led_.reset_glow();
        freeze_led_.reset_glow();
        mode_ = NORMAL;         // calibration abort
      }
    } break;

    case CALIBRATION_SLOPE: {
      if (control_.CalibrateSlope()) {
        learn_led_.flash(Colors::white);
        freeze_led_.flash(Colors::white);
      }
      learn_led_.reset_glow();
      freeze_led_.reset_glow();
      mode_ = NORMAL;
    } break;
    }
  }

public:
  Ui() {
    // Initialize switches to their current positions
    Base::put({SwitchGridSwitched, switches_.get_grid()});
    Base::put({SwitchModSwitched, switches_.get_mod()});
    Base::put({SwitchTwistSwitched, switches_.get_twist()});
    Base::put({SwitchWarpSwitched, switches_.get_warp()});

    // Enter calibration if Learn is pushed
    if (buttons_.learn_.pushed()) {
      mode_ = CALIBRATION_OFFSET;
      learn_led_.set_glow(Colors::red, 2_f);
      freeze_led_.set_glow(Colors::red, 2_f);
    }
  }

  PolypticOscillator<block_size>& osc() { return osc_; }

  void Poll(Block<Frame, block_size> codec_in) {
    control_.ProcessCodecInput(codec_in);
    Base::Poll();
  }

  void Update() {
    learn_led_.Update();
    freeze_led_.Update();
  }
};
