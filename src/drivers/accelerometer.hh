#include "stm32f4xx.h"

#define LIS3DSH_OUT_T_ADDR                   0x0C
#define LIS3DSH_INFO1_ADDR                   0x0D
#define LIS3DSH_INFO2_ADDR                   0x0E
#define LIS3DSH_WHO_AM_I_ADDR                0x0F
#define LIS3DSH_OFF_X_ADDR                   0x10
#define LIS3DSH_OFF_Y_ADDR                   0x11
#define LIS3DSH_OFF_Z_ADDR                   0x12
#define LIS3DSH_CS_X_ADDR                    0x13
#define LIS3DSH_CS_Y_ADDR                    0x14
#define LIS3DSH_CS_Z_ADDR                    0x15
#define LIS3DSH_LC_L_ADDR                    0x16
#define LIS3DSH_LC_H_ADDR                    0x17
#define LIS3DSH_STAT_ADDR                    0x18
#define LIS3DSH_PEAK1_ADDR                   0x19
#define LIS3DSH_PEAK2_ADDR                   0x1A
#define LIS3DSH_VFC_1_ADDR                   0x1B
#define LIS3DSH_VFC_2_ADDR                   0x1C
#define LIS3DSH_VFC_3_ADDR                   0x1D
#define LIS3DSH_THRS3_ADDR                   0x1F
#define LIS3DSH_CTRL_REG4_ADDR               0x20
#define LIS3DSH_CTRL_REG1_ADDR               0x21
#define LIS3DSH_CTRL_REG2_ADDR               0x22
#define LIS3DSH_CTRL_REG3_ADDR               0x23
#define LIS3DSH_CTRL_REG5_ADDR               0x24
#define LIS3DSH_CTRL_REG6_ADDR               0x25
#define LIS3DSH_STATUS_ADDR                  0x27
#define LIS3DSH_OUT_X_L_ADDR                 0x28
#define LIS3DSH_OUT_X_H_ADDR                 0x29
#define LIS3DSH_OUT_Y_L_ADDR                 0x2A
#define LIS3DSH_OUT_Y_H_ADDR                 0x2B
#define LIS3DSH_OUT_Z_L_ADDR                 0x2C
#define LIS3DSH_OUT_Z_H_ADDR                 0x2D
#define LIS3DSH_FIFO_CTRL_ADDR               0x2E
#define LIS3DSH_FIFO_SRC_ADDR                0x2F
#define LIS3DSH_ST1_1_ADDR                   0x40
#define LIS3DSH_ST1_2_ADDR                   0x41
#define LIS3DSH_ST1_3_ADDR                   0x42
#define LIS3DSH_ST1_4_ADDR                   0x43
#define LIS3DSH_ST1_5_ADDR                   0x44
#define LIS3DSH_ST1_6_ADDR                   0x45
#define LIS3DSH_ST1_7_ADDR                   0x46
#define LIS3DSH_ST1_8_ADDR                   0x47
#define LIS3DSH_ST1_9_ADDR                   0x48
#define LIS3DSH_ST1_10_ADDR                  0x49
#define LIS3DSH_ST1_11_ADDR                  0x4A
#define LIS3DSH_ST1_12_ADDR                  0x4B
#define LIS3DSH_ST1_13_ADDR                  0x4C
#define LIS3DSH_ST1_14_ADDR                  0x4D
#define LIS3DSH_ST1_15_ADDR                  0x4E
#define LIS3DSH_ST1_16_ADDR                  0x4F
#define LIS3DSH_TIM4_1_ADDR                  0x50
#define LIS3DSH_TIM3_1_ADDR                  0x51
#define LIS3DSH_TIM2_1_L_ADDR                0x52
#define LIS3DSH_TIM2_1_H_ADDR                0x53
#define LIS3DSH_TIM1_1_L_ADDR                0x54
#define LIS3DSH_TIM1_1_H_ADDR                0x55
#define LIS3DSH_THRS2_1_ADDR                 0x56
#define LIS3DSH_THRS1_1_ADDR                 0x57
#define LIS3DSH_MASK1_B_ADDR                 0x59
#define LIS3DSH_MASK1_A_ADDR                 0x5A
#define LIS3DSH_SETT1_ADDR                   0x5B
#define LIS3DSH_PR1_ADDR                     0x5C
#define LIS3DSH_TC1_L_ADDR                   0x5D
#define LIS3DSH_TC1_H_ADDR                   0x5E
#define LIS3DSH_OUTS1_ADDR                   0x5F
#define LIS3DSH_ST2_16_ADDR                  0x6F
#define LIS3DSH_TIM4_2_ADDR                  0x70
#define LIS3DSH_TIM3_2_ADDR                  0x71
#define LIS3DSH_TIM2_2_L_ADDR                0x72
#define LIS3DSH_TIM2_2_H_ADDR                0x73
#define LIS3DSH_TIM1_2_L_ADDR                0x74
#define LIS3DSH_TIM1_2_H_ADDR                0x75
#define LIS3DSH_THRS2_2_ADDR                 0x76
#define LIS3DSH_THRS1_2_ADDR                 0x77
#define LIS3DSH_DES2_ADDR                    0x78
#define LIS3DSH_MASK2_B_ADDR                 0x79
#define LIS3DSH_MASK2_A_ADDR                 0x7A
#define LIS3DSH_SETT2_ADDR                   0x7B
#define LIS3DSH_PR2_ADDR                     0x7C
#define LIS3DSH_TC2_L_ADDR                   0x7D
#define LIS3DSH_TC2_H_ADDR                   0x7E
#define LIS3DSH_OUTS2_ADDR                   0x7F

class Accelerometer {

  SPI_HandleTypeDef hspi_;

  void WriteCs(bool b) {
    if (b) HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);
    else HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);
  }

  uint8_t WriteRead(uint8_t b) {
    uint8_t r = 0;
    if (HAL_SPI_TransmitReceive(&hspi_, &b, &r, 1, 1000) != HAL_OK)
      while(1);
    return r;
  }

#define READWRITE_CMD                     ((uint8_t)0x80) 
#define MULTIPLEBYTE_CMD                  ((uint8_t)0x40)

  // write data into a register of the accelerometer
  void Write(uint8_t reg, uint8_t *data, uint16_t size) {
    if(size > 1) {
      reg |= MULTIPLEBYTE_CMD;
    }
    WriteCs(false);
    WriteRead(reg);
    while(size--) {
      WriteRead(*data);
      data++;
    }
    WriteCs(true);
  }

  // read data from one (or several) register(s)
  void Read(uint8_t reg, uint8_t *data, uint16_t size) {
    reg |= READWRITE_CMD;
    if(size > 1) reg |= MULTIPLEBYTE_CMD;

    WriteCs(false);
    WriteRead(reg);
    /* Receive the data that will be read from the device (MSB First) */
    uint8_t in[size] = {0};
    if (HAL_SPI_TransmitReceive(&hspi_, in, data, size, 1000) != HAL_OK) {
      while(1);
    }
    WriteCs(true);
  }

public:
  Accelerometer() {
    /* Init I2C1 */

    // SCK (PA5), PA6 (MISO), PA7 (MOSI), pin config
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_MEDIUM;
    gpio.Pull = GPIO_PULLDOWN;
    gpio.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &gpio);

    // PE3 (CS) pin config
    __HAL_RCC_GPIOE_CLK_ENABLE();
    gpio.Pin = GPIO_PIN_3;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_MEDIUM;
    HAL_GPIO_Init(GPIOE, &gpio);

    // RESET = SPI mode, I2C disabled
    WriteCs(false);

    // SPI1 config
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_SPI1_FORCE_RESET();
    __HAL_RCC_SPI1_RELEASE_RESET();

    hspi_.Instance = SPI1;
    hspi_.Init.Mode = SPI_MODE_MASTER;
    hspi_.Init.Direction = SPI_DIRECTION_2LINES;
    hspi_.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi_.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi_.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi_.Init.NSS = SPI_NSS_SOFT;
    hspi_.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    hspi_.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi_.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi_.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;

    HAL_SPI_Init(&hspi_);

    // Chip configuration:
    uint8_t ctrl;
    ctrl = 0b01111111;  // On at 100Hz, BDU active, XYZ active
    Write(LIS3DSH_CTRL_REG4_ADDR, &ctrl, 1);
    ctrl = 0b11000000;  // antialias filter at 50Hz, +/-2g scale, 4w-SPI
    Write(LIS3DSH_CTRL_REG5_ADDR, &ctrl, 1);
    ctrl = 0b00010000;  // address increment on
    Write(LIS3DSH_CTRL_REG6_ADDR, &ctrl, 1);
    ctrl = 0b00000000;  // FIFO bypass mode
    Write(LIS3DSH_FIFO_CTRL_ADDR, &ctrl, 1);
  }

  struct AccelData {
    short x;
    short y;
    short z;
  };

  void ReadAccelData(AccelData *d) {
    Read(LIS3DSH_OUT_X_L_ADDR, ((uint8_t*)&d->x), 1);
    Read(LIS3DSH_OUT_X_H_ADDR, ((uint8_t*)&d->x)+1, 1);
    Read(LIS3DSH_OUT_Y_L_ADDR, ((uint8_t*)&d->y), 1);
    Read(LIS3DSH_OUT_Y_H_ADDR, ((uint8_t*)&d->y)+1, 1);
    Read(LIS3DSH_OUT_Z_L_ADDR, ((uint8_t*)&d->z), 1);
    Read(LIS3DSH_OUT_Z_H_ADDR, ((uint8_t*)&d->z)+1, 1);
  }

};
