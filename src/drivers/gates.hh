#pragma once

struct Gates : Nocopy {

  template<class T>
  struct Debouncer : crtp<T, Debouncer> {
    uint8_t state_;
    void Debounce() { state_ = (state_ << 1) | (**this).get(); }
    bool just_disabled() const { return state_ == 0b01111111; }
    bool just_enabled() const { return state_ == 0b10000000; }
    bool enabled() const { return state_ == 0b00000000; }
  };

  struct Learn : Debouncer<Learn> {
    uint8_t state_;
    Learn() {
      __HAL_RCC_GPIOE_CLK_ENABLE();
      GPIO_InitTypeDef gpio = {0};
      gpio.Pin = GPIO_PIN_7;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLDOWN;
      HAL_GPIO_Init(GPIOE, &gpio);
    }
    bool get() { return !ReadPin(GPIOE, GPIO_PIN_7); };
  } learn_;

  struct Freeze : Debouncer<Freeze> {
    uint8_t state_;
    Freeze() {
      __HAL_RCC_GPIOB_CLK_ENABLE();
      GPIO_InitTypeDef gpio = {0};
      gpio.Pin = GPIO_PIN_2;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLDOWN;
      HAL_GPIO_Init(GPIOB, &gpio);
    }
    bool get() { return ReadPin(GPIOB, GPIO_PIN_2); };
  } freeze_;

  void Debounce() {
    learn_.Debounce();
    freeze_.Debounce();
  }
};
