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
  enum State { UP, CENTER, DOWN, ERROR };

  struct Grid {
    Grid() {
      __HAL_RCC_GPIOB_CLK_ENABLE();
      GPIO_InitTypeDef gpio = {0};
      gpio.Pin = GRIDSW_TOP_Pin|GRIDSW_BOT_Pin;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(GRIDSW_TOP_GPIO_Port, &gpio);
    }
    bool get1() { return HAL_GPIO_ReadPin(GRIDSW_TOP_GPIO_Port, GRIDSW_TOP_Pin); }
    bool get2() { return HAL_GPIO_ReadPin(GRIDSW_BOT_GPIO_Port, GRIDSW_BOT_Pin); }
    State get() {
      return get1() && get2() ? CENTER :
        get1() ? DOWN :
        get2() ? UP :
        ERROR;
    }
  } grid_;

  struct Mod {
    Mod() {
      __HAL_RCC_GPIOE_CLK_ENABLE();
      GPIO_InitTypeDef gpio = {0};
      gpio.Pin = MODSW_TOP_Pin|MODSW_BOT_Pin;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(MODSW_TOP_GPIO_Port, &gpio);
    }
    bool get1() { return HAL_GPIO_ReadPin(MODSW_TOP_GPIO_Port, MODSW_TOP_Pin); }
    bool get2() { return HAL_GPIO_ReadPin(MODSW_BOT_GPIO_Port, MODSW_BOT_Pin); }
    State get() {
      return get1() && get2() ? CENTER :
        get1() ? DOWN :
        get2() ? UP :
        ERROR;
    }
  } mod_;

  struct Twist {
    Twist() {
      __HAL_RCC_GPIOD_CLK_ENABLE();
      GPIO_InitTypeDef gpio = {0};
      gpio.Pin = TWISTSW_TOP_Pin|TWISTSW_BOT_Pin;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(TWISTSW_TOP_GPIO_Port, &gpio);
    }
    bool get1() { return HAL_GPIO_ReadPin(TWISTSW_TOP_GPIO_Port, TWISTSW_TOP_Pin); }
    bool get2() { return HAL_GPIO_ReadPin(TWISTSW_BOT_GPIO_Port, TWISTSW_BOT_Pin); }
    State get() {
      return get1() && get2() ? CENTER :
        get1() ? DOWN :
        get2() ? UP :
        ERROR;
    }
  } twist_;

  struct Warp {
    Warp() {
      __HAL_RCC_GPIOC_CLK_ENABLE();
      GPIO_InitTypeDef gpio = {0};
      gpio.Pin = WARPSW_TOP_Pin|WARPSW_BOT_Pin;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(WARPSW_TOP_GPIO_Port, &gpio);
    }
    bool get1() { return HAL_GPIO_ReadPin(WARPSW_TOP_GPIO_Port, WARPSW_TOP_Pin); }
    bool get2() { return HAL_GPIO_ReadPin(WARPSW_BOT_GPIO_Port, WARPSW_BOT_Pin); }
    State get() {
      return get1() && get2() ? CENTER :
        get1() ? DOWN :
        get2() ? UP :
        ERROR;
    }
  } warp_;
};
