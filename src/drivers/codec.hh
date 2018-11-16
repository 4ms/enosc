
#pragma once

#include "parameters.hh"
#include "hal.hh"

#define CODEC_IS_SLAVE 0
#define CODEC_IS_MASTER 1

#define MCLK_SRC_STM 0
#define MCLK_SRC_EXTERNAL 1

#define W8731_ADDR_0 0x1A
#define W8731_ADDR_1 0x1B
#define W8731_NUM_REGS 10

//Set configuration here:
#define CODEC_MODE 				CODEC_IS_SLAVE
#define CODEC_MCLK_SRC 			MCLK_SRC_STM
#define CODEC_ADDRESS           (W8731_ADDR_0<<1)

struct Codec {

  struct Callback {
    virtual void Process(int32_t* in, int32_t *out, int size) = 0;
  };

  Codec(Callback *callback, int sample_rate) :
    callback_(callback) {
    instance_ = this;

    // Setup PLL clock for codec
    init_SAI_clock(sample_rate);

   	//De-init the codec to force it to reset
    i2c_.DeInit();

    //Start Codec I2C
    gpio_.Init();
    i2c_.Init(CODEC_MODE, sample_rate);

    //Start Codec SAI
    SAI_init(sample_rate);
    init_audio_DMA();
  }

  void Start();

  static Codec *instance_;
  Callback *callback_;

  DMA_HandleTypeDef hdma_sai1b_rx;
  DMA_HandleTypeDef hdma_sai1a_tx;

  uint32_t tx_buffer_start, rx_buffer_start, tx_buffer_half, rx_buffer_half;

private:

  struct GPIO {
    void Init();
  } gpio_;

  struct I2C {
    void Init(uint8_t master_slave, uint32_t sample_rate);
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

private:

  SAI_HandleTypeDef hsai1b_rx;
  SAI_HandleTypeDef hsai1a_tx;

  volatile int32_t tx_buffer[kBlockSize * 2];
  volatile int32_t rx_buffer[kBlockSize * 2];
};
