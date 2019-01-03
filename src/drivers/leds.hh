#pragma once

#include "hal.hh"
#include "dsp.hh"

#define FREEZE_LED_PWM_TIM TIM1
#define LEARN_LED_PWM_TIM TIM3

#define FREEZE_LED_PWM_CC_RED 		CCR1
#define FREEZE_LED_PWM_CC_GREEN 	CCR2
#define FREEZE_LED_PWM_CC_BLUE 		CCR3

#define LEARN_LED_PWM_CC_RED 		CCR1
#define LEARN_LED_PWM_CC_GREEN 		CCR2
#define LEARN_LED_PWM_CC_BLUE 		CCR3

struct Color {
  explicit constexpr Color(u0_8 r, u0_8 g, u0_8 b) : r_(r), g_(g), b_(b) {}
  u0_8 red() { return r_; }
  u0_8 green() { return g_; }
  u0_8 blue() { return b_; }
  constexpr const Color operator+(Color const that) const {
    return Color(this->r_.add_sat(that.r_),
                 this->g_.add_sat(that.g_),
                 this->b_.add_sat(that.b_));
  }
  const Color blend(Color const that) const {
    return Color(this->r_.div2<1>() + that.r_.div2<1>(),
                 this->g_.div2<1>() + that.g_.div2<1>(),
                 this->b_.div2<1>() + that.b_.div2<1>());
  }
  constexpr const Color blend(Color const that, u0_8 const phase) const {
    u0_8 r = u0_8::narrow(phase * that.r_ + (u0_8::max_val - phase) * this->r_);
    u0_8 g = u0_8::narrow(phase * that.g_ + (u0_8::max_val - phase) * this->g_);
    u0_8 b = u0_8::narrow(phase * that.b_ + (u0_8::max_val - phase) * this->b_);
    return Color(u0_8(r), u0_8(g), u0_8(b));
  }
private:
  u0_8 r_, g_, b_;
};

struct Colors {
  static constexpr Color black = Color(0._u0_8, 0._u0_8, 0._u0_8);
  static constexpr Color white = Color(u0_8::max_val, u0_8::max_val, u0_8::max_val);
  static constexpr Color red = Color(u0_8::max_val, 0._u0_8, 0._u0_8);
  static constexpr Color green = Color(0._u0_8, u0_8::max_val, 0._u0_8);
  static constexpr Color blue = Color(0._u0_8, 0._u0_8, u0_8::max_val);
  static constexpr Color yellow = Color(u0_8::max_val, u0_8::max_val, 0._u0_8);
  static constexpr Color magenta = Color(u0_8::max_val, 0._u0_8, u0_8::max_val);
  static constexpr Color cyan = Color(0._u0_8, u0_8::max_val, u0_8::max_val);
};

struct Leds : Nocopy {
  Leds();
  
  template<class T>
  struct ILed : crtp<T, ILed> {
    void set(u0_8 r, u0_8 g, u0_8 b) { return (**this).set(r, g, b); }
    void set(Color c) { set(c.red(), c.green(), c.blue()); }
  };

  struct Freeze : public ILed<Freeze> {
    void set(u0_8 r, u0_8 g, u0_8 b) {
      FREEZE_LED_PWM_TIM->FREEZE_LED_PWM_CC_RED 	= r.repr();
      FREEZE_LED_PWM_TIM->FREEZE_LED_PWM_CC_GREEN = g.repr();
      FREEZE_LED_PWM_TIM->FREEZE_LED_PWM_CC_BLUE 	= b.repr();
    }
  } freeze_;

  struct Learn : public ILed<Learn> {
    void set(u0_8 r, u0_8 g, u0_8 b) {
      LEARN_LED_PWM_TIM->LEARN_LED_PWM_CC_RED 	= r.repr();
      LEARN_LED_PWM_TIM->LEARN_LED_PWM_CC_GREEN = b.repr(); // WARNING:
                                                            // inverted here!
      LEARN_LED_PWM_TIM->LEARN_LED_PWM_CC_BLUE 	= g.repr();
    }
  } learn_;

private:
  TIM_HandleTypeDef	timFREEZELED;
  TIM_HandleTypeDef	timLEARNLED;
};
