#include "stm32f4xx.h"
#include "parameters.hh"

enum I2S_Freq {
  I2S_FREQ_8000 = 0,
  I2S_FREQ_11025 = 1,
  I2S_FREQ_16000 = 2,
  I2S_FREQ_22050 = 3,
  I2S_FREQ_32000 = 4,
  I2S_FREQ_44100 = 5,
  I2S_FREQ_48000 = 6,
  I2S_FREQ_96000 = 7,
  I2S_ERROR = -1,
};

class Dac
{
  ShortFrame block_[kBlockSize * 2] = {zero};

  static constexpr int kPeriphI2CAddress = 0b10010100;

  static constexpr uint8_t kCS43L22_REG_ID = 0x01;
  static constexpr uint8_t kCS43L22_REG_POWER_CTL1 = 0x02;
  static constexpr uint8_t kCS43L22_REG_POWER_CTL2 = 0x04;
  static constexpr uint8_t kCS43L22_REG_CLOCKING_CTL = 0x05;
  static constexpr uint8_t kCS43L22_REG_INTERFACE_CTL1 = 0x06;
  static constexpr uint8_t kCS43L22_REG_INTERFACE_CTL2 = 0x07;
  static constexpr uint8_t kCS43L22_REG_PASSTHR_A_SELECT = 0x08;
  static constexpr uint8_t kCS43L22_REG_PASSTHR_B_SELECT = 0x09;
  static constexpr uint8_t kCS43L22_REG_ANALOG_ZC_SR_SETT = 0x0A;
  static constexpr uint8_t kCS43L22_REG_PASSTHR_GANG_CTL = 0x0C;
  static constexpr uint8_t kCS43L22_REG_PLAYBACK_CTL1 = 0x0D;
  static constexpr uint8_t kCS43L22_REG_MISC_CTL = 0x0E;
  static constexpr uint8_t kCS43L22_REG_PLAYBACK_CTL2 = 0x0F;
  static constexpr uint8_t kCS43L22_REG_PASSTHR_A_VOL = 0x14;
  static constexpr uint8_t kCS43L22_REG_PASSTHR_B_VOL = 0x15;
  static constexpr uint8_t kCS43L22_REG_PCMA_VOL = 0x1A;
  static constexpr uint8_t kCS43L22_REG_PCMB_VOL = 0x1B;
  static constexpr uint8_t kCS43L22_REG_BEEP_FREQ_ON_TIME = 0x1C;
  static constexpr uint8_t kCS43L22_REG_BEEP_VOL_OFF_TIME = 0x1D;
  static constexpr uint8_t kCS43L22_REG_BEEP_TONE_CFG = 0x1E;
  static constexpr uint8_t kCS43L22_REG_TONE_CTL = 0x1F;
  static constexpr uint8_t kCS43L22_REG_MASTER_A_VOL = 0x20;
  static constexpr uint8_t kCS43L22_REG_MASTER_B_VOL = 0x21;
  static constexpr uint8_t kCS43L22_REG_HEADPHONE_A_VOL = 0x22;
  static constexpr uint8_t kCS43L22_REG_HEADPHONE_B_VOL = 0x23;
  static constexpr uint8_t kCS43L22_REG_SPEAKER_A_VOL = 0x24;
  static constexpr uint8_t kCS43L22_REG_SPEAKER_B_VOL = 0x25;
  static constexpr uint8_t kCS43L22_REG_CH_MIXER_SWAP = 0x26;
  static constexpr uint8_t kCS43L22_REG_LIMIT_CTL1 = 0x27;
  static constexpr uint8_t kCS43L22_REG_LIMIT_CTL2 = 0x28;
  static constexpr uint8_t kCS43L22_REG_LIMIT_ATTACK_RATE = 0x29;
  static constexpr uint8_t kCS43L22_REG_OVF_CLK_STATUS = 0x2E;
  static constexpr uint8_t kCS43L22_REG_BATT_COMPENSATION = 0x2F;
  static constexpr uint8_t kCS43L22_REG_VP_BATTERY_LEVEL = 0x30;
  static constexpr uint8_t kCS43L22_REG_SPEAKER_STATUS = 0x31;
  static constexpr uint8_t kCS43L22_REG_TEMPMONITOR_CTL = 0x32;
  static constexpr uint8_t kCS43L22_REG_THERMAL_FOLDBACK = 0x33;
  static constexpr uint8_t kCS43L22_REG_CHARGE_PUMP_FREQ = 0x34;

public:

  static I2C_HandleTypeDef  hi2c_;
  static I2S_HandleTypeDef  hi2s_;
  static DMA_HandleTypeDef hdma_;
  
  struct ProcessCallback {
    virtual void Process(ShortFrame* out, int size) = 0;
  };

private:
  ProcessCallback *callback_;

public:

  static Dac* instance_;

  Dac(ProcessCallback *callback) :
    callback_(callback) {
    instance_ = this;
    Init_I2C();
    Init_DAC();
    Init_I2S();
    Init_DMA();
  }

  void Transmit(uint8_t reg, uint8_t data) {
    uint8_t d[2] = {reg, data};
    if (HAL_I2C_Master_Transmit(&hi2c_, kPeriphI2CAddress, d, 2, 100) != HAL_OK) {
      while(1);
    }
  }

  uint8_t Receive(uint8_t reg) {
    uint8_t d = 0;
    if (HAL_I2C_Master_Transmit(&hi2c_, kPeriphI2CAddress, &reg, 1, 100) != HAL_OK)
      while(1);
    if (HAL_I2C_Master_Receive(&hi2c_, kPeriphI2CAddress, &d, 1, 100) != HAL_OK)
      while(1);
    return d;
  }

  void Init_I2C() {
    hi2c_.Instance = I2C1;
    hi2c_.Init.OwnAddress1 = 0x33;
    hi2c_.Init.ClockSpeed = 100000U;
    hi2c_.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c_.Init.DutyCycle = I2C_DUTYCYCLE_2;

    HAL_I2C_DeInit(&hi2c_);

    /* Init reset pin */
    __HAL_RCC_GPIOD_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_4;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FAST;
    gpio.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOD, &gpio);

    /* Init I2C1 */

    // SDA (PB9) & SCL (PB6) pins config
    __HAL_RCC_GPIOB_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Pull = GPIO_NOPULL;
    gpio.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &gpio);

    // i2c1 config
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_I2C1_FORCE_RESET();
    __HAL_RCC_I2C1_RELEASE_RESET();

    HAL_I2C_Init(&hi2c_);

    // reset DAC
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_Delay(10);
  }

  void Init_I2S() {

    constexpr I2S_Freq freq =
      kSampleRate == 8000 ? I2S_FREQ_8000 :
      kSampleRate == 11025 ? I2S_FREQ_11025 :
      kSampleRate == 16000 ? I2S_FREQ_16000 :
      kSampleRate == 22050 ? I2S_FREQ_22050 :
      kSampleRate == 32000 ? I2S_FREQ_32000 :
      kSampleRate == 44100 ? I2S_FREQ_44100 :
      kSampleRate == 48000 ? I2S_FREQ_48000 :
      kSampleRate == 96000 ? I2S_FREQ_96000 :
      kSampleRate == 8000 ? I2S_FREQ_8000 :
      I2S_ERROR;

    const uint32_t I2SFreq[8] = {8000, 11025, 16000, 22050, 32000, 44100, 48000, 96000};
    const uint32_t I2SPLLN[8] = {258, 429, 213, 429, 426, 271, 260, 344};
    const uint32_t I2SPLLR[8] = {3, 4, 4, 4, 4, 6, 3, 2};

    // Enable PLLI2S clock
    RCC_PeriphCLKInitTypeDef rcc;
    HAL_RCCEx_GetPeriphCLKConfig(&rcc);
    rcc.PeriphClockSelection = RCC_PERIPHCLK_I2S;
    rcc.PLLI2S.PLLI2SN = I2SPLLN[freq];
    rcc.PLLI2S.PLLI2SR = I2SPLLR[freq];
    HAL_RCCEx_PeriphCLKConfig(&rcc);

    // enable I2S3 clock
    __HAL_RCC_SPI3_CLK_ENABLE();

    // configure GPIOs:
    // SD=PC12, SCK=PC10, MCK=PC7, WS=PA4
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef  gpio;
    gpio.Pin         = GPIO_PIN_10 | GPIO_PIN_12 | GPIO_PIN_7;
    gpio.Mode        = GPIO_MODE_AF_PP;
    gpio.Pull        = GPIO_NOPULL;
    gpio.Speed       = GPIO_SPEED_LOW;
    gpio.Alternate   = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOC, &gpio);

    gpio.Pin = GPIO_PIN_4;
    HAL_GPIO_Init(GPIOA, &gpio);

    hi2s_.Instance = SPI3;
    __HAL_I2S_DISABLE(&hi2s_);

    hi2s_.Init.AudioFreq   = I2SFreq[freq];
    hi2s_.Init.ClockSource = I2S_CLOCK_PLL;
    hi2s_.Init.CPOL        = I2S_CPOL_LOW;
    hi2s_.Init.DataFormat  = I2S_DATAFORMAT_16B;
    hi2s_.Init.MCLKOutput  = I2S_MCLKOUTPUT_ENABLE;
    hi2s_.Init.Mode        = I2S_MODE_MASTER_TX;
    hi2s_.Init.Standard    = I2S_STANDARD_PHILIPS;

    if (HAL_I2S_Init(&hi2s_) != HAL_OK)
      while(1);
  }

  void Init_DMA() {
    // // enable the I2S DMA clock
    __HAL_RCC_DMA1_CLK_ENABLE();

    hdma_.Instance = DMA1_Stream7;
    hdma_.Init.Channel = DMA_CHANNEL_0;
    hdma_.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_.Init.MemInc = DMA_MINC_ENABLE;
    hdma_.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_.Init.Mode = DMA_CIRCULAR;
    hdma_.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    hdma_.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma_.Init.MemBurst = DMA_MBURST_SINGLE;
    hdma_.Init.PeriphBurst = DMA_PBURST_SINGLE;

    __HAL_LINKDMA(&hi2s_, hdmatx, hdma_);

    HAL_DMA_DeInit(&hdma_);
    HAL_DMA_Init(&hdma_);

    /* I2S DMA IRQ Channel configuration */
    HAL_NVIC_SetPriority(DMA1_Stream7_IRQn, 0x0F, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream7_IRQn);
  }

  void Init_DAC() {
    set_power(true);
    set_output_device(OD_HEADPHONE);
    set_clocking(CLK_AUTO);
    set_interface();
    set_volume(130);
  }

  void Start() {
    uint16_t *data = (uint16_t*)block_;
    if (HAL_I2S_Transmit_DMA(&hi2s_, data, kBlockSize * 2 * 2) != HAL_OK)
      while(1);
  }

  void Stop() {
    if (HAL_I2S_DMAStop(&hi2s_) != HAL_OK)
      while(1);
  }

  void set_power(bool status) {
    uint8_t msg = status ? 0b10011110 : 0b00000001;
    Transmit(kCS43L22_REG_POWER_CTL1, msg);
  }

  enum OutputDevice {
    OD_SPEAKER = 0xFA,
    OD_HEADPHONE = 0xAF,
    OD_BOTH = 0xAA, OD_AUTO = 0x05
  };

  enum Clocking { CLK_AUTO = 0x81, };

  void set_output_device(OutputDevice od) {
    Transmit(kCS43L22_REG_POWER_CTL2, od);
  }

  void set_clocking(Clocking c) {
    Transmit(kCS43L22_REG_CLOCKING_CTL, c);
  }

  void set_interface() {
    // slave mode, dsp disabled, i2s standard, 16 bits
    Transmit(kCS43L22_REG_INTERFACE_CTL1, 0b00000111);
    Transmit(kCS43L22_REG_INTERFACE_CTL2, 0b00000000);

    Transmit(kCS43L22_REG_PASSTHR_A_SELECT, 0b00000000);
    Transmit(kCS43L22_REG_PASSTHR_B_SELECT, 0b00000000);

    Transmit(kCS43L22_REG_PLAYBACK_CTL1, 0b00010000);
    Transmit(kCS43L22_REG_MISC_CTL, 0b00000000);

    Transmit(kCS43L22_REG_PLAYBACK_CTL2, 0b00000000);

    Transmit(kCS43L22_REG_PASSTHR_A_VOL, 0b10000000);
    Transmit(kCS43L22_REG_PASSTHR_B_VOL, 0b10000000);
}

  void Process(int offset) {
    ShortFrame *out = &block_[offset * kBlockSize];
    callback_->Process(out, kBlockSize);
  }

  // 0..255, from -100dB to +12dB
  void set_volume(uint8_t vol) {
    vol += 25;
    Transmit(kCS43L22_REG_MASTER_A_VOL, vol);
    Transmit(kCS43L22_REG_MASTER_B_VOL, vol);
  }
};

extern "C" {
  void DMA1_Stream7_IRQHandler() {
    HAL_DMA_IRQHandler(&Dac::hdma_);
  }

  void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s_) {
    Dac::instance_->Process(0);
  }

  void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s_) {
    Dac::instance_->Process(1);
  }
}

Dac* Dac::instance_;
I2C_HandleTypeDef Dac::hi2c_;
I2S_HandleTypeDef Dac::hi2s_;
DMA_HandleTypeDef Dac::hdma_;
