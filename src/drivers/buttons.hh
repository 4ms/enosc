#include <cstdint>

struct Buttons {
  struct Button {
    uint8_t state_;
    virtual bool get() = 0;
    void Debounce() { state_ = (state_ << 1) | get(); }
    bool released(uint8_t index) const { return state_ == 0b01111111; }
    bool just_pressed(uint8_t index) const { return state_ == 0x10000000; }
    bool pressed(uint8_t index) const { return state_ == 0b00000000; }
  };

  struct Learn : Button {
    Learn() {
      __HAL_RCC_GPIOC_CLK_ENABLE();
      GPIO_InitTypeDef gpio;
      gpio.Pin = GPIO_PIN_9;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(GPIOC, &gpio);
    }
    bool get() { return HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_9); };
  } learn;

  struct Freeze : Button {
    Freeze() {
      __HAL_RCC_GPIOA_CLK_ENABLE();
      GPIO_InitTypeDef gpio;
      gpio.Pin = GPIO_PIN_11;
      gpio.Mode = GPIO_MODE_INPUT;
      gpio.Pull = GPIO_PULLUP;
      HAL_GPIO_Init(GPIOA, &gpio);
    }
    bool get() { return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_11); };
  } freeze;

  void Debounce() {
    learn.Debounce();
    freeze.Debounce();
  }
};
