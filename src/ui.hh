#pragma once

#include "buttons.hh"
#include "switches.hh"
#include "leds.hh"
#include "control.hh"
#include "polyptic_oscillator.hh"
#include "event_handler.hh"
#include "bitfield.hh"

constexpr u1_7 kLedAdjustMin = 0.5_u1_7;
constexpr u1_7 kLedAdjustMax = 1.5_u1_7;

constexpr u1_7 kLedAdjustValidMin = kLedAdjustMin.pred();
constexpr u1_7 kLedAdjustValidMax = kLedAdjustMax.succ();
constexpr f kLedAdjustRange = (f)(kLedAdjustMax - kLedAdjustMin);
constexpr f kLedAdjustOffset = (f)(kLedAdjustMin);

template<int update_rate, class T>
struct LedManager : Leds::ILed<T> {

  LedManager(Color::Adjustment& color_cal) : color_cal_(color_cal) {}

  // flash_freq in Hz; max = update_rate
  void flash(Color c, f flash_freq = 10_f) {
    flash_color_ = c;
    flash_phase_ = max_val<u0_16>;
    flash_freq_ = u0_16(flash_freq / f(update_rate));
  }

  void set_background(Color c) { background_color_ = c; }
  void set_solid(Color c) { solid_color_ = c; }

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
    if (solid_color_ != Colors::black) c = solid_color_;
    c = c.blend(glow_color_, u0_8::narrow(osc_.Process()));
    c = c.blend(flash_color_, u0_8::narrow(flash_phase_));
    c = c.adjust(color_cal_);
    Leds::ILed<T>::set(c);
    if (flash_phase_ > flash_freq_) flash_phase_ -= flash_freq_;
    else flash_phase_ = 0._u0_16;
  }

  void set_cal(f r, f g, f b) {
    color_cal_.r = u1_7(r);
    color_cal_.g = u1_7(g);
    color_cal_.b = u1_7(b);
  }

private:
  TriangleOscillator osc_;
  Color background_color_ = Colors::black;
  Color solid_color_ = Colors::black;
  Color flash_color_ = Colors::white;
  Color glow_color_ = Colors::red;
  u0_16 flash_freq_ = 0.0014_u0_16;
  u0_16 flash_phase_ = 0._u0_16;
  Color::Adjustment& color_cal_;
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

  SwitchEventSource<Scale, SwitchScale> scale_;
  SwitchEventSource<Mod, SwitchMod> mod_;
  SwitchEventSource<Twist, SwitchTwist> twist_;
  SwitchEventSource<Warp, SwitchWarp> warp_;

  void Poll(std::function<void(Event)> const& put) {
    scale_.Poll(put);
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

  Persistent<WearLevel<FlashBlock<1, Parameters::AltParameters>>>
  alt_params_ {&params_.alt, params_.default_alt};

  static constexpr int kProcessRate = kSampleRate / block_size;
  static constexpr int kLongPressTime = 4.0f * kProcessRate; // sec
  static constexpr int kNewNoteDelayTime = 0.01f * kProcessRate; // sec

  f cached_pitch_base_;

  struct LedCalibrationData {
    Color::Adjustment led_learn_adjust;
    Color::Adjustment led_freeze_adjust;

    bool validate() {
      return
        led_learn_adjust.r <= kLedAdjustValidMax && led_learn_adjust.r >= kLedAdjustValidMin &&
        led_learn_adjust.g <= kLedAdjustValidMax && led_learn_adjust.g >= kLedAdjustValidMin &&
        led_learn_adjust.b <= kLedAdjustValidMax && led_learn_adjust.b >= kLedAdjustValidMin &&
        led_freeze_adjust.r <= kLedAdjustValidMax && led_freeze_adjust.r >= kLedAdjustValidMin &&
        led_freeze_adjust.g <= kLedAdjustValidMax && led_freeze_adjust.g >= kLedAdjustValidMin &&
        led_freeze_adjust.b <= kLedAdjustValidMax && led_freeze_adjust.b >= kLedAdjustValidMin;
    }
  };
  LedCalibrationData led_calibration_data_;
  LedCalibrationData default_led_calibration_data_ = {
    {1._u1_7, 1._u1_7, 1._u1_7},
    {1._u1_7, 1._u1_7, 1._u1_7}
  };

  Persistent<WearLevel<FlashBlock<3, LedCalibrationData>>>
  led_calibration_data_storage_ {&led_calibration_data_, default_led_calibration_data_};

  LedManager<update_rate, Leds::Learn> learn_led_ {led_calibration_data_.led_learn_adjust};
  LedManager<update_rate, Leds::Freeze> freeze_led_ {led_calibration_data_.led_freeze_adjust};

  typename Base::DelayedEventSource button_timeouts_[2];
  typename Base::DelayedEventSource new_note_delay_;
  ButtonsEventSource buttons_;
  SwitchesEventSource switches_;
  Control<block_size> control_ {params_, osc_};

  EventSource<Event>* sources_[6] = {
    &buttons_, &switches_,
    &button_timeouts_[0], &button_timeouts_[1],
    &control_, &new_note_delay_
  };

  enum Mode {
    NORMAL,
    SHIFT,
    LEARN,
    MANUAL_LEARN,
    CALIBRATE_CV,
    CALIBRATE_LEDS,
  } mode_ = NORMAL;

  Bitfield<32> active_catchups_ {0};

  void reset_leds() {
    learn_led_.set_background(Colors::lemon);
    freeze_led_.set_background(Colors::lemon);
    learn_led_.reset_glow();
    freeze_led_.reset_glow();
  }


  void Handle(typename Base::EventStack stack) {
    Event& e1 = stack.get(0);
    Event& e2 = stack.get(1);

    switch(e1.type) {
    case StartCatchup: {
      active_catchups_ = active_catchups_.set(e1.data);
      freeze_led_.set_glow(Colors::grey, 2_f * f(active_catchups_.set_bits()));
    } break;
    case EndOfCatchup: {
      active_catchups_ = active_catchups_.reset(e1.data);
      freeze_led_.flash(Colors::green);
      freeze_led_.reset_glow();
      freeze_led_.set_glow(Colors::grey, 2_f * f(active_catchups_.set_bits()));
    } break;
    case ScaleChange: {
      if (mode_==NORMAL || mode_==LEARN || mode_==MANUAL_LEARN)
        learn_led_.flash(Colors::white);
    } break;
    case ButtonPush: {
      button_timeouts_[e1.data].trigger_after(kLongPressTime, {ButtonTimeout, e1.data});
    } break;
    case SwitchScale: {
      params_.scale.mode =
        e1.data == Switches::UP ? TWELVE :
        e1.data == Switches::MID ? OCTAVE : FREE;
    } break;
    case SwitchMod: {
      params_.modulation.mode =
        e1.data == Switches::UP ? ONE :
        e1.data == Switches::MID ? TWO : THREE;
    } break;
    case SwitchTwist: {
      params_.twist.mode =
        e1.data == Switches::UP ? FEEDBACK :
        e1.data == Switches::MID ? PULSAR : CRUSH;
    } break;
    case SwitchWarp: {
      params_.warp.mode =
        e1.data == Switches::UP ? FOLD :
        e1.data == Switches::MID ? CHEBY : SEGMENT;
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
            learn_led_.set_solid(Colors::dark_red);
            cached_pitch_base_ = osc_.lowest_pitch() - control_.pitch_cv();
            osc_.enable_learn();
            control_.hold_pitch_cv();
          }
        }
      } break;
      case ButtonTimeout: {
        if (e1.data == BUTTON_LEARN &&
            e2.type == ButtonPush &&
            e2.data == BUTTON_LEARN) {
          // long-press on Learn
          osc_.reset_current_scale();
          control_.all_main_function();
          learn_led_.flash(Colors::blue, 2_f);
        }
      } break;
      case ButtonPush: {
        if (e1.data == BUTTON_FREEZE) {
          mode_ = SHIFT;
          //stop updating pot values when the Shift button 
          //goes down, in case the user performs an alt function
          control_.disable_all_alt_shift_pot_values();
        }
        if (e1.data == BUTTON_LEARN) 
          control_.disable_all_alt_learn_pot_values();
      } break;
      case PotMove: {
        if (e1.data == POT_ROOT &&
            e2.type == ButtonPush &&
            e2.data == BUTTON_LEARN) {
          // manually add a first note
          learn_led_.set_solid(Colors::dark_red);
          osc_.enable_learn();
          f cur_pitch = osc_.lowest_pitch();
          osc_.new_note(cur_pitch);
          learn_led_.flash(Colors::white);
          mode_ = MANUAL_LEARN;
          learn_led_.set_glow(Colors::red, 3_f);
          osc_.enable_pre_listen();
          osc_.enable_follow_new_note();
          control_.root_pot_alternate_function();
          control_.pitch_pot_alternate_function();
        }
      } break;
      }
    } break;

    case SHIFT: {
      switch(e1.type) {
      case PotMove: {
        if (e1.data == POT_SPREAD) {
          freeze_led_.set_solid(Colors::grey);
          control_.spread_pot_alternate_function();
        } else if (e1.data == POT_TWIST) {
          freeze_led_.set_solid(Colors::grey);
          control_.twist_pot_alternate_function();
        } else if (e1.data == POT_WARP) {
          freeze_led_.set_solid(Colors::grey);
          control_.warp_pot_alternate_function();
        } else if (e1.data == POT_BALANCE) {
          freeze_led_.set_solid(Colors::grey);
          control_.balance_pot_alternate_function();
        } else if (e1.data == POT_PITCH) {
          freeze_led_.set_solid(Colors::cyan);
          control_.pitch_pot_alternate_function();
        }
      } break;
      case ButtonRelease: {
        if (e1.data == BUTTON_FREEZE) {
          if (e2.type == ButtonPush &&
              e2.data == BUTTON_FREEZE) {
            // Freeze pressed
            osc_.set_freeze(!osc_.frozen());
            freeze_led_.set_solid(osc_.frozen() ? Colors::blue : Colors::black);
            mode_ = NORMAL;
            control_.all_main_function();
          } else {
            // Released after a change
            mode_ = NORMAL;
            control_.all_main_function();
			params_.alt.pitch_pot_state = control_.pitch_pot_state();
            alt_params_.Save();
            freeze_led_.set_solid(osc_.frozen() ? Colors::blue : Colors::black);
          }
        }
      } break;
      case ButtonTimeout: {
        if (e2.type == ButtonTimeout &&
            e1.data != e2.data) {
          // long-press on Learn and Freeze
          mode_ = CALIBRATE_CV;
          learn_led_.set_background(Colors::black);
          freeze_led_.set_background(Colors::black);
          control_.calibration_reset();
          control_.next_calibration();
        }
      } break;
      case AltParamChange: {
        freeze_led_.flash(Colors::white);
      } break;
      }
    } break;

    case LEARN: {
      switch(e1.type) {
      case NewNoteAfterDelay: {
        new_note_delay_.trigger_after(kNewNoteDelayTime, {NewNote, 0});
      } break;
      case NewNote: {
        bool success = osc_.new_note(control_.pitch_cv() + cached_pitch_base_);
        osc_.enable_pre_listen();
        learn_led_.flash(success ? Colors::white : Colors::black);
      } break;
      case PotMove: {
        if (e1.data == POT_ROOT &&
            e2.type == ButtonPush &&
            e2.data == BUTTON_LEARN) {
          // manually add notes
          if (osc_.empty_pre_scale()) {
            // if scale is empty, add note with current pitch
            f cur_pitch = osc_.lowest_pitch();
            osc_.new_note(cur_pitch);
          }
          if (osc_.new_note(0_f)) {
            learn_led_.flash(Colors::white);
            mode_ = MANUAL_LEARN;
            learn_led_.set_glow(Colors::red, 3_f);
            osc_.enable_pre_listen();
            osc_.enable_follow_new_note();
            control_.root_pot_alternate_function();
            control_.pitch_pot_reset_alternate_value();
          } else {
            learn_led_.flash(Colors::black);
          }
        } else if (e1.data == POT_PITCH &&
                   e2.type == ButtonPush &&
                   e2.data == BUTTON_LEARN) {
          mode_ = MANUAL_LEARN;
          control_.pitch_pot_alternate_function();
          osc_.enable_follow_new_note();
        }
      } break;
      case ButtonRelease: {
        if (e1.data == BUTTON_LEARN &&
            e2.type == ButtonPush &&
            e2.data == BUTTON_LEARN) {
          // Learn pressed
          mode_ = NORMAL;
          bool success = osc_.disable_learn();
          if (success) learn_led_.flash(Colors::green, 2_f);
          control_.release_pitch_cv();
          control_.all_main_function();
          learn_led_.reset_glow();
          learn_led_.set_solid(Colors::black);
        } else if (e1.data == BUTTON_FREEZE &&
            e2.type == ButtonPush &&
            e2.data == BUTTON_FREEZE) {
          // Freeze pressed
          bool success = osc_.remove_last_note();
          if (success) freeze_led_.flash(Colors::black);
        }
      } break;
      }
    } break;

    case MANUAL_LEARN: {
      if (e1.type == ButtonRelease && e1.data == BUTTON_LEARN) {
        osc_.disable_follow_new_note();
        mode_ = LEARN;
      }
      if (e1.type == PotMove && e1.data == POT_PITCH) {
          control_.pitch_pot_alternate_function();
      }
    } break;

    case CALIBRATE_CV: {
      if (e1.type == CalibrationFailed) {
        learn_led_.flash(Colors::magenta, 0.5_f); 
        freeze_led_.flash(Colors::magenta, 0.5_f);
        reset_leds();
        mode_ = NORMAL;
      }
      else
      if (e1.type == CalibrationStepDone) {
        switch (control_.calibration_state()) {
          case CALIBRATING_PITCH_UNPATCHED:
            control_.next_calibration(); //special-case: go directly to next step
            break;
          case CALIBRATING_ROOT_UNPATCHED:
            learn_led_.set_glow(Colors::blue, 2_f);
            freeze_led_.reset_glow();
            break;
          case CALIBRATING_PITCH_OFFSET:
            learn_led_.set_glow(Colors::blue, 6_f);
            freeze_led_.reset_glow();
            break;
          case CALIBRATING_PITCH_SLOPE:
            learn_led_.reset_glow();
            freeze_led_.set_glow(Colors::blue, 2_f);
            break;
          case CALIBRATING_ROOT_OFFSET:
            learn_led_.reset_glow();
            freeze_led_.set_glow(Colors::blue, 6_f);
            break;
          case CALIBRATING_ROOT_SLOPE:
            if (control_.calibrate_offsets()) {
              control_.SaveCalibration();
              learn_led_.flash(Colors::green, 0.5_f); //success
              freeze_led_.flash(Colors::green, 0.5_f);
            } else {
              learn_led_.flash(Colors::magenta, 0.5_f); //fail
              freeze_led_.flash(Colors::magenta, 0.5_f);
            }
            reset_leds();
            mode_ = NORMAL;
            break;
        }
      }      
      else //wait for button press to advance to next step
      if (e1.type == ButtonRelease &&
          e2.type == ButtonPush &&
          e1.data == e2.data &&
          !control_.calibration_busy()
        ){
        learn_led_.set_glow(Colors::dark_cyan, 10_f); //busy lights
        freeze_led_.set_glow(Colors::dark_cyan, 10_f);
        control_.next_calibration();
      }
    } break;

    case CALIBRATE_LEDS: {
      if (e1.type == ButtonRelease &&
          e1.data == BUTTON_FREEZE &&
          e2.type == ButtonPush &&
          e2.data == BUTTON_FREEZE) {
        mode_ = NORMAL; //Freeze => save led calibration
        learn_led_.set_background(Colors::lemon);
        freeze_led_.set_background(Colors::lemon);
        led_calibration_data_storage_.Save();
      } else {
        learn_led_.set_cal(control_.scale_pot()*kLedAdjustRange + kLedAdjustOffset,
                           control_.balance_pot()*kLedAdjustRange + kLedAdjustOffset,
                           control_.twist_pot()*kLedAdjustRange + kLedAdjustOffset);

        freeze_led_.set_cal(control_.pitch_pot()*kLedAdjustRange + kLedAdjustOffset,
                            control_.modulation_pot()*kLedAdjustRange + kLedAdjustOffset,
                            control_.warp_pot()*kLedAdjustRange + kLedAdjustOffset);
        if (switches_.scale_.get()==1) { //Scale switch selects color to use for calibrating
          learn_led_.set_background(Colors::lemon);
          freeze_led_.set_background(Colors::lemon);
        } else {
          learn_led_.set_background(Colors::grey50);
          freeze_led_.set_background(Colors::grey50);
        }
      }
    } break;

    }
  }

public:
  Ui() {
    // Initialize switches to their current positions
    Base::put({SwitchScale, switches_.scale_.get()});
    Base::put({SwitchMod, switches_.mod_.get()});
    Base::put({SwitchTwist, switches_.twist_.get()});
    Base::put({SwitchWarp, switches_.warp_.get()});
    Base::Process();


    // Enter LED calibration if Freeze is pushed and all switches are centered
    if (buttons_.freeze_.pushed() &&
               switches_.scale_.get()==3 &&
               switches_.mod_.get()==3 &&
               switches_.twist_.get()==3 &&
               switches_.warp_.get()==3) {
      mode_ = CALIBRATE_LEDS;
    } else {
      mode_ = NORMAL;
      learn_led_.set_background(Colors::lemon);
      freeze_led_.set_background(Colors::lemon);
    }
  }

  PolypticOscillator<block_size>& osc() { return osc_; }

  void Poll() {
    control_.ProcessSpiAdcInput();
    Base::Poll();
    freeze_led_.set_solid(osc_.frozen() ? Colors::blue : Colors::black);
  }
  
  void Update() {
    learn_led_.Update();
    freeze_led_.Update();
  }
};
