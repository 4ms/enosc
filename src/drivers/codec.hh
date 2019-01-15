#pragma once

#include "buffer.hh"
#include "parameters.hh"
#include "hal.hh"
#include <functional>

struct Codec : Nocopy {

  using callback_t = const std::function<void(DoubleBlock<Frame, Frame, kBlockSize> inout)>;

  Codec(int sample_rate, callback_t &callback) :
    callback_(callback) {
    instance_ = this;

    // Setup PLL clock for codec
    init_SAI_clock(sample_rate);

   	//De-init the codec to force it to reset
    i2c_.DeInit();
    HAL_Delay(10);

    //Start Codec I2C
    gpio_.Init();
    HAL_Delay(100);
    i2c_.Init(sample_rate);
    HAL_Delay(100);

    //Start Codec SAI
    SAI_init(sample_rate);
    init_audio_DMA();
  }

  void Start();

  static Codec *instance_;
  callback_t &callback_;

  DMA_HandleTypeDef hdma_rx;
  DMA_HandleTypeDef hdma_tx;

  volatile int16_t tx_buffer[kBlockSize * 2 * 2];
  volatile int16_t rx_buffer[kBlockSize * 2 * 2];

private:

  struct GPIO {
    // TODO init with constructor
    void Init();
  } gpio_;

  struct I2C {
    void Init(uint32_t sample_rate);
    void DeInit();
    void PowerDown();
  private:
    void Write(uint8_t addr, uint16_t value);
    I2C_HandleTypeDef handle_;
  } i2c_;

  void Reboot(uint32_t sample_rate);

  // TODO cleanup: SAI & DMA
  void init_SAI_clock(uint32_t sample_rate);
  void SAI_init(uint32_t sample_rate);
  void DeInit_I2S_Clock();
  void DeInit_SAIDMA();
  void init_audio_DMA();

  SAI_HandleTypeDef hsai_rx;
  SAI_HandleTypeDef hsai_tx;

};
