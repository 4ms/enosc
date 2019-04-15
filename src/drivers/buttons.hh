#pragma once

#include <cstdint>

struct Buttons : Nocopy {

  template<class T>
  struct Debouncer : crtp<T, Debouncer> {
    uint8_t state_ = 0xFF;
    void Debounce() {
      state_ = (state_ << 1) | (**this).get();
    }
    bool just_released() const {
      return state_ == 0b01111111;
    }
    bool just_pressed() const {
      return state_ == 0b10000000;
    }
    bool pressed() const {
      return state_ == 0b00000000;
    }
  };

  struct Learn : public Debouncer<Learn> {
    Learn() {
      __HAL_RCC_GPIOC_CLK_ENABLE();
      GPIO_InitTypeDef gpio = {0};
      gpio.Pin = GPIO_PIN_9;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(GPIOC, &gpio);
    }
    bool get() { return ReadPin(GPIOC, GPIO_PIN_9); };
  } learn_;

  struct Freeze : Debouncer<Freeze> {
    Freeze() {
      __HAL_RCC_GPIOA_CLK_ENABLE();
      GPIO_InitTypeDef gpio = {0};
      gpio.Pin = GPIO_PIN_11;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(GPIOA, &gpio);
    }
    bool get() { return ReadPin(GPIOA, GPIO_PIN_11); };
  } freeze_;

  void Debounce() {
    learn_.Debounce();
    freeze_.Debounce();
  }
};
