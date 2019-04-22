#pragma once

#include "hal.hh"

#define MODSW_TOP_Pin GPIO_PIN_14
#define MODSW_TOP_GPIO_Port GPIOE
#define MODSW_BOT_Pin GPIO_PIN_15
#define MODSW_BOT_GPIO_Port GPIOE

#define GRIDSW_TOP_Pin GPIO_PIN_12
#define GRIDSW_TOP_GPIO_Port GPIOB
#define GRIDSW_BOT_Pin GPIO_PIN_13
#define GRIDSW_BOT_GPIO_Port GPIOB

#define TWISTSW_TOP_Pin GPIO_PIN_14
#define TWISTSW_TOP_GPIO_Port GPIOD
#define TWISTSW_BOT_Pin GPIO_PIN_15
#define TWISTSW_BOT_GPIO_Port GPIOD

#define WARPSW_TOP_Pin GPIO_PIN_11 /*reversed from PCB*/
#define WARPSW_TOP_GPIO_Port GPIOC
#define WARPSW_BOT_Pin GPIO_PIN_10 /*reversed from PCB*/
#define WARPSW_BOT_GPIO_Port GPIOC

struct Switches : Nocopy {
  enum State { UP=1, DOWN=2, CENTER=3 };

  template<class T>
  struct Combiner : crtp<T, Combiner<T>> {
    uint32_t state_ = 0;
    void Debounce() {
      state_ = (state_ << 2) | (**this).get2() | ((**this).get1() << 1);
    }
    bool just_switched() {
      uint16_t s = state_ ^ (state_ >> 2);
      return (s & 0b11 == 0) && (s & 0b1100 != 0);
    }
    State get() { return static_cast<State>(state_ & 0b11); }
  };

  struct Grid : Combiner<Grid> {
    Grid() {
      __HAL_RCC_GPIOB_CLK_ENABLE();
      GPIO_InitTypeDef gpio = {0};
      gpio.Pin = GRIDSW_TOP_Pin|GRIDSW_BOT_Pin;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(GRIDSW_TOP_GPIO_Port, &gpio);
    }
    bool get1() { return ReadPin(GRIDSW_TOP_GPIO_Port, GRIDSW_TOP_Pin); }
    bool get2() { return ReadPin(GRIDSW_BOT_GPIO_Port, GRIDSW_BOT_Pin); }
  } grid_;

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
      gpio.Pin = WARPSW_TOP_Pin|WARPSW_BOT_Pin;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(WARPSW_TOP_GPIO_Port, &gpio);
    }
    bool get1() { return ReadPin(WARPSW_TOP_GPIO_Port, WARPSW_TOP_Pin); }
    bool get2() { return ReadPin(WARPSW_BOT_GPIO_Port, WARPSW_BOT_Pin); }
  } warp_;

  void Debounce() {
    grid_.Debounce();
    mod_.Debounce();
    twist_.Debounce();
    warp_.Debounce();
  }
};
