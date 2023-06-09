#pragma once

#include "hal.hh"

#define MODSW_TOP_Pin GPIO_PIN_14
#define MODSW_TOP_GPIO_Port GPIOE
#define MODSW_BOT_Pin GPIO_PIN_15
#define MODSW_BOT_GPIO_Port GPIOE

#define SCALESW_TOP_Pin GPIO_PIN_10
#define SCALESW_TOP_GPIO_Port GPIOB
#define SCALESW_BOT_Pin GPIO_PIN_11
#define SCALESW_BOT_GPIO_Port GPIOB

#define TWISTSW_TOP_Pin GPIO_PIN_14
#define TWISTSW_TOP_GPIO_Port GPIOD
#define TWISTSW_BOT_Pin GPIO_PIN_15
#define TWISTSW_BOT_GPIO_Port GPIOD

#define WARPSW_TOP_Pin GPIO_PIN_15
#define WARPSW_TOP_GPIO_Port GPIOA
#define WARPSW_BOT_Pin GPIO_PIN_10
#define WARPSW_BOT_GPIO_Port GPIOC

struct Switches : Nocopy {

  Switches() {
    for(int i=0; i<16; i++) Debounce();
  }

  enum Switch { SCALE, MOD, TWIST, WARP };
  enum State { UP=1, DOWN=2, MID=3 };

  template<class T>
  struct Combiner : crtp<T, Combiner<T>> {
    uint8_t state_ = 0x00;
    void Debounce() {
      state_ = (state_ << 2) | (**this).get2() | ((**this).get1() << 1);
    }
    bool just_switched_mid() {
      return
        state_ == 0b01111111 ||
        state_ == 0b10111111;
    }
    bool just_switched_up() {
      return state_ == 0b11010101;
    }
    bool just_switched_down() {
      return state_ == 0b11101010;
    }
    State get() {
      Debounce();
      return static_cast<State>(state_ & 0b11);
    }
  };

  struct Scale : Combiner<Scale> {
    Scale() {
      __HAL_RCC_GPIOB_CLK_ENABLE();
      GPIO_InitTypeDef gpio = {0};
      gpio.Pin = SCALESW_TOP_Pin|SCALESW_BOT_Pin;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(SCALESW_TOP_GPIO_Port, &gpio);
    }
    bool get1() { return ReadPin(SCALESW_TOP_GPIO_Port, SCALESW_TOP_Pin); }
    bool get2() { return ReadPin(SCALESW_BOT_GPIO_Port, SCALESW_BOT_Pin); }
  } scale_;

  struct Mod : Combiner<Mod> {
    Mod() {
      __HAL_RCC_GPIOE_CLK_ENABLE();
      GPIO_InitTypeDef gpio = {0};
      gpio.Pin = MODSW_TOP_Pin|MODSW_BOT_Pin;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(MODSW_TOP_GPIO_Port, &gpio);
    }
    bool get1() { return ReadPin(MODSW_TOP_GPIO_Port, MODSW_TOP_Pin); }
    bool get2() { return ReadPin(MODSW_BOT_GPIO_Port, MODSW_BOT_Pin); }
  } mod_;

  struct Twist : Combiner<Twist> {
    Twist() {
      __HAL_RCC_GPIOD_CLK_ENABLE();
      GPIO_InitTypeDef gpio = {0};
      gpio.Pin = TWISTSW_TOP_Pin|TWISTSW_BOT_Pin;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(TWISTSW_TOP_GPIO_Port, &gpio);
    }
    bool get1() { return ReadPin(TWISTSW_TOP_GPIO_Port, TWISTSW_TOP_Pin); }
    bool get2() { return ReadPin(TWISTSW_BOT_GPIO_Port, TWISTSW_BOT_Pin); }
  } twist_;

  struct Warp : Combiner<Warp> {
    Warp() {
      __HAL_RCC_GPIOC_CLK_ENABLE();
      GPIO_InitTypeDef gpio = {0};
      gpio.Pin = WARPSW_TOP_Pin;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(WARPSW_TOP_GPIO_Port, &gpio);
      gpio.Pin = WARPSW_BOT_Pin;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(WARPSW_BOT_GPIO_Port, &gpio);
    }
    bool get1() { return ReadPin(WARPSW_TOP_GPIO_Port, WARPSW_TOP_Pin); }
    bool get2() { return ReadPin(WARPSW_BOT_GPIO_Port, WARPSW_BOT_Pin); }
  } warp_;

  void Debounce() {
    scale_.Debounce();
    mod_.Debounce();
    twist_.Debounce();
    warp_.Debounce();
  }
};
