#pragma once

#include <variant>
#include "buttons.hh"
#include "gates.hh"
#include "switches.hh"
#include "leds.hh"
#include "control.hh"
#include "polyptic_oscillator.hh"
#include "event_handler.hh"
#include "bitfield.hh"


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

template<class Switch, EventType event>
struct SwitchEventSource : EventSource<Event>, Switch {
  void Poll(std::function<void(Event)> const& put) {
    Switch::Debounce();
    if (Switch::just_switched_up()) put({event, Switches::UP});
    else if (Switch::just_switched_mid()) put({event, Switches::MID});
    else if (Switch::just_switched_down()) put({event, Switches::DOWN});
  }
};

struct SwitchesEventSource : EventSource<Event>, Switches {

  SwitchEventSource<Grid, SwitchGrid> grid_;
  SwitchEventSource<Mod, SwitchMod> mod_;
  SwitchEventSource<Twist, SwitchTwist> twist_;
  SwitchEventSource<Warp, SwitchWarp> warp_;

  void Poll(std::function<void(Event)> const& put) {
    grid_.Poll(put);
    mod_.Poll(put);
    twist_.Poll(put);
    warp_.Poll(put);
  }
};

template<int update_rate, int block_size>
class Ui : public EventHandler<Ui<update_rate, block_size>, Event> {
  using Base = EventHandler<Ui, Event>;
  friend Base;

  Parameters params_;
  Leds leds_;
  PolypticOscillator<block_size> osc_ {params_};

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
  Control<block_size> control_ {params_};

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

  Bitfield<32> active_catchups_ {0};

  void Handle(typename Base::EventStack stack) {
    Event& e1 = stack.get(0);
    Event& e2 = stack.get(1);

    switch(e1.type) {
    case GateOn: {
      if (e1.data == GATE_FREEZE)
        osc_.set_freeze(!osc_.frozen());
      freeze_led_.set_background(osc_.frozen() ? Colors::blue : Colors::black);
    } break;
    case GateOff: {
      if (e1.data == GATE_FREEZE)
        osc_.set_freeze(!osc_.frozen());
      freeze_led_.set_background(osc_.frozen() ? Colors::blue : Colors::black);
    } break;
    case StartCatchup: {
      active_catchups_ = active_catchups_.set(e1.data);
      freeze_led_.set_glow(Colors::grey, 2_f * f(active_catchups_.set_bits()));
    } break;
    case EndOfCatchup: {
      active_catchups_ = active_catchups_.reset(e1.data);
      freeze_led_.reset_glow();
      freeze_led_.set_glow(Colors::grey, 2_f * f(active_catchups_.set_bits()));
    } break;
    case GridChange: {
      learn_led_.flash(Colors::white);
    } break;
    case ButtonPush: {
      button_timeouts_[e1.data].trigger_after(kLongPressTime, {ButtonTimeout, e1.data});
    } break;
    }
    
    switch(mode_) {

    case NORMAL: {

      switch(e1.type) {
      case ButtonRelease: {
        if (e2.type == ButtonPush &&
            e1.data == e2.data) {
          // Learn pressed
          if (e1.data == BUTTON_LEARN) {
            mode_ = LEARN;
            learn_led_.set_background(Colors::dark_red);
            osc_.enable_learn();
            control_.hold_pitch_cv();
          }
        }
      } break;
      case ButtonPush: {
        if (e1.data == BUTTON_FREEZE) {
          mode_ = SHIFT;
        }
      } break;
      case SwitchGrid: {
        params_.grid.mode =
          e1.data == Switches::UP ? TWELVE :
          e1.data == Switches::MID ? OCTAVE : FREE;
        Base::put({EndOfCatchup, ADC_INPUT_MAX + Switches::GRID});
      } break;
      case SwitchMod: {
        params_.modulation.mode =
          e1.data == Switches::UP ? ONE :
          e1.data == Switches::MID ? TWO : THREE;
        Base::put({EndOfCatchup, ADC_INPUT_MAX + Switches::MOD});
      } break;
      case SwitchTwist: {
        params_.twist.mode =
          e1.data == Switches::UP ? FEEDBACK :
          e1.data == Switches::MID ? PULSAR : DECIMATE;
        Base::put({EndOfCatchup, ADC_INPUT_MAX + Switches::TWIST});
      } break;
      case SwitchWarp: {
        params_.warp.mode =
          e1.data == Switches::UP ? FOLD :
          e1.data == Switches::MID ? CHEBY : CRUSH;
        Base::put({EndOfCatchup, ADC_INPUT_MAX + Switches::WARP});
      } break;
      }
    } break;

    case SHIFT: {
      switch(e1.type) {
      case SwitchGrid: {
        freeze_led_.flash(Colors::white);
        freeze_led_.set_background(Colors::grey);
        params_.crossfade_factor =
          e1.data == Switches::UP ? Crossfade::linear :
          e1.data == Switches::MID ? Crossfade::mid : Crossfade::steep;
        Base::put({StartCatchup, ADC_INPUT_MAX+Switches::GRID});
      } break;
      case SwitchTwist: {
        freeze_led_.flash(Colors::white);
        freeze_led_.set_background(Colors::grey);
        params_.stereo_mode =
          e1.data == Switches::UP ? ALTERNATE :
          e1.data == Switches::MID ? LOW_HIGH : LOWEST_REST;
        Base::put({StartCatchup, ADC_INPUT_MAX+Switches::TWIST});
      } break;
      case SwitchWarp: {
        freeze_led_.flash(Colors::white);
        freeze_led_.set_background(Colors::grey);
        params_.freeze_mode =
          e1.data == Switches::UP ? ALTERNATE :
          e1.data == Switches::MID ? LOW_HIGH : LOWEST_REST;
        Base::put({StartCatchup, ADC_INPUT_MAX+Switches::WARP});
      } break;
      case PotMove: {
        if (e1.data == POT_SPREAD) {
          freeze_led_.set_background(Colors::grey);
          control_.spread_pot_alternate_function();
        } else if (e1.data == POT_TWIST) {
          freeze_led_.set_background(Colors::grey);
          control_.twist_pot_alternate_function();
        }
      }
      case ButtonRelease: {
        if (e1.data == BUTTON_FREEZE) {
          if (e2.type == ButtonPush &&
              e2.data == BUTTON_FREEZE) {
            // Freeze pressed
            osc_.set_freeze(!osc_.frozen());
            freeze_led_.set_background(osc_.frozen() ? Colors::blue : Colors::black);
            mode_ = NORMAL;
          } else {
            // Released after a change
            mode_ = NORMAL;
            control_.all_main_function();
            freeze_led_.set_background(osc_.frozen() ? Colors::blue : Colors::black);
          }
        }
      } break;
      case AltParamChange: {
        freeze_led_.flash(Colors::white);
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
        bool success = osc_.new_note(control_.pitch_cv());
        learn_led_.flash(success ? Colors::white : Colors::black);
      } break;
      case PotMove: {
        if (e1.data == POT_ROOT &&
            e2.type == ButtonPush &&
            e2.data == BUTTON_LEARN) {
          if (osc_.new_note(0_f)) {
            learn_led_.flash(Colors::white);
            mode_ = MANUAL_LEARN;
            learn_led_.set_glow(Colors::red, 3_f);
            osc_.enable_pre_listen();
            osc_.enable_follow_new_note();
            control_.root_pot_alternate_function();
            control_.pitch_pot_alternate_function();
          } else {
            learn_led_.flash(Colors::black);
          }
        }
      } break;
      case ButtonRelease: {
        if (e1.data == BUTTON_LEARN &&
            e2.type == ButtonPush &&
            e2.data == BUTTON_LEARN) {
          // Learn pressed
          mode_ = NORMAL;
          bool success = osc_.disable_learn();
          if (success) learn_led_.flash(Colors::green);
          control_.release_pitch_cv();
          learn_led_.reset_glow();
          learn_led_.set_background(Colors::black);
        } else if (e1.data == BUTTON_FREEZE &&
            e2.type == ButtonPush &&
            e2.data == BUTTON_FREEZE) {
          // Freeze pressed
          bool success = osc_.remove_last_note();
          if (success) freeze_led_.flash(Colors::magenta);
        }
      } break;
      }
    } break;

    case MANUAL_LEARN: {

      if (e1.type == ButtonRelease && e1.data == BUTTON_LEARN) {
        control_.root_pot_main_function();
        control_.pitch_pot_main_function();
        osc_.disable_follow_new_note();
        mode_ = LEARN;
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
      if (e1.type == ButtonRelease &&
          e1.data == BUTTON_LEARN &&
          e2.type == ButtonPush &&
          e2.data == BUTTON_LEARN) {
        if (control_.CalibrateSlope()) {
          learn_led_.flash(Colors::white);
          freeze_led_.flash(Colors::white);
        }
        learn_led_.reset_glow();
        freeze_led_.reset_glow();
        mode_ = NORMAL;
      }
    } break;
    }
  }

public:
  Ui() {
    // Initialize switches to their current positions
    Base::put({SwitchGrid, switches_.grid_.get()});
    Base::put({SwitchMod, switches_.mod_.get()});
    Base::put({SwitchTwist, switches_.twist_.get()});
    Base::put({SwitchWarp, switches_.warp_.get()});
    Base::Process();

    // Enter calibration if Learn is pushed
    if (buttons_.learn_.pushed()) {
      mode_ = CALIBRATION_OFFSET;
      learn_led_.set_glow(Colors::red, 2_f);
      freeze_led_.set_glow(Colors::red, 2_f);
    }
  }

  PolypticOscillator<block_size>& osc() { return osc_; }

  void Poll(Buffer<Frame, block_size>& codec_in) {
    control_.ProcessCodecInput(codec_in);
    Base::Poll();
  }

  void Update() {
    learn_led_.Update();
    freeze_led_.Update();
  }
};
