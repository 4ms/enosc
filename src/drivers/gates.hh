#pragma once

enum Gate {
  GATE_LEARN,
  GATE_FREEZE,
};

#define FREEZE_JACK_Pin GPIO_PIN_9
#define FREEZE_JACK_GPIO_Port GPIOB
#define FREEZE_JACK_RCC_CLK_ON __HAL_RCC_GPIOB_CLK_ENABLE

#define LEARN_JACK_Pin GPIO_PIN_8
#define LEARN_JACK_GPIO_Port GPIOB
#define LEARN_JACK_RCC_CLK_ON __HAL_RCC_GPIOB_CLK_ENABLE


struct Gates : Nocopy {

  template<class T>
  struct Debouncer : crtp<T, Debouncer<T>> {
    uint8_t state_;
    void Debounce() { state_ = (state_ << 1) | (**this).get(); }
    bool just_disabled() const { return state_ == 0b01111111; }
    bool just_enabled() const { return state_ == 0b10000000; }
    bool enabled() const { return state_ == 0b00000000; }
  };

  struct Learn : Debouncer<Learn> {
    uint8_t state_;
    Learn() {
      LEARN_JACK_RCC_CLK_ON();
      GPIO_InitTypeDef gpio = {0};
      gpio.Pin = LEARN_JACK_Pin;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLDOWN;
      HAL_GPIO_Init(LEARN_JACK_GPIO_Port, &gpio);
    }
    bool get() { return !ReadPin(LEARN_JACK_GPIO_Port, LEARN_JACK_Pin); };
  } learn_;

  struct Freeze : Debouncer<Freeze> {
    uint8_t state_;
    Freeze() {
      FREEZE_JACK_RCC_CLK_ON();
      GPIO_InitTypeDef gpio = {0};
      gpio.Pin = FREEZE_JACK_Pin;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLDOWN;
      HAL_GPIO_Init(FREEZE_JACK_GPIO_Port, &gpio);
    }
    bool get() { return ReadPin(FREEZE_JACK_GPIO_Port, FREEZE_JACK_Pin); };
  } freeze_;

  void Debounce() {
    learn_.Debounce();
    freeze_.Debounce();
  }
};
