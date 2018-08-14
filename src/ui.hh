#include "drivers/leds.hh"
#include "drivers/button.hh"

class Ui {
  Leds leds_;
  Button button_;

  static constexpr int kModeNr = 4;
  static constexpr int kLongPressTime = 1000;

  int mode_ = 0;

  // 0=not pressed; n=pressed since n ms
  int button_press_time_ = 0;

public:
  void Poll() {
    button_.Read();
    if (button_.just_pressed()) {
      button_press_time_ = 1;
      onButtonPress();
    } else if (button_.just_released()) {
      button_press_time_ = 0;
      onButtonRelease();
    }
    if (button_press_time_ > 0) button_press_time_++;

    if (button_press_time_ > kLongPressTime) {
      onLongButtonPress();
      button_press_time_ = 1;
    }
  }

  void onButtonPress() { mode_ = (mode_ + 1) % kModeNr; }
  void onButtonRelease() {}
  void onLongButtonPress() { mode_ = 0; }

  void Display() {
    for (int i=0; i<kModeNr; i++)
        leds_.set((LED)i, i==mode_);
  }

  int mode() { return mode_; }
};
