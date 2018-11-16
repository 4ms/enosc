/*
 * codec_i2c.h: setup and init for WM8731 codec
 *
 * Author: Dan Green (danngreen1@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * See http://creativecommons.org/licenses/MIT/ for more information.
 *
 * -----------------------------------------------------------------------------
 */

#pragma once

#include "parameters.hh"
#include "hal.hh"

/* DMA rx/tx buffer size, in number of DMA Periph/MemAlign-sized elements (words) */
#define codec_BUFF_LEN 1024


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
    deinit();

    //Start Codec I2C
    GPIO_init();
    I2C_init();

    if (register_setup(sample_rate))
      assert_failed(__FILE__, __LINE__);

    //Start Codec SAI
    SAI_init(sample_rate);
    init_audio_DMA();
  }

  static Codec *instance_;
  Callback *callback_;

  DMA_HandleTypeDef hdma_sai1b_rx;
  DMA_HandleTypeDef hdma_sai1a_tx;

  uint32_t tx_buffer_start, rx_buffer_start, tx_buffer_half, rx_buffer_half;

  void Start(void);

private:

  void reboot(uint32_t sample_rate);
  void deinit(void);

  // i2c
  uint32_t power_down(void);
  uint32_t register_setup(uint32_t sample_rate);
  void GPIO_init(void);
  void I2C_init(void);

  // sai
  void init_SAI_clock(uint32_t sample_rate);
  void SAI_init(uint32_t sample_rate);
  void Init_SAIDMA(void);
  void DeInit_I2S_Clock(void);
  void DeInit_SAIDMA(void);
  void init_audio_DMA(void);
  uint32_t reset(uint8_t master_slave, uint32_t sample_rate);

private:

  uint32_t write_register(uint8_t RegisterAddr, uint16_t RegisterValue);
  I2C_HandleTypeDef codec_i2c;

  SAI_HandleTypeDef hsai1b_rx;
  SAI_HandleTypeDef hsai1a_tx;

  volatile int32_t tx_buffer[codec_BUFF_LEN];
  volatile int32_t rx_buffer[codec_BUFF_LEN];
};
