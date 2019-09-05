
#pragma once

class QSpiFlash {

public:

  enum ErasableSizes {
    SECTOR,
    BLOCK_32K,
    BLOCK_64K,
    ENTIRE_CHIP
  };

  enum FlashStatus {
    STATUS_READY,
    STATUS_WIP,
    STATUS_RXING,
    STATUS_RX_COMPLETE,
    STATUS_TXING,
    STATUS_TX_COMPLETE
  };

  enum UseInterruptFlags {
    EXECUTE_FOREGROUND,
    EXECUTE_BACKGROUND
  };

  // public for use in callbacks and IRQ
  volatile enum FlashStatus status = STATUS_READY;
  QSPI_HandleTypeDef handle;

private:

  QSPI_CommandTypeDef s_command;

  uint8_t WriteEnable(void);
  void GPIO_Init_1IO(void);
  void GPIO_Init_IO2_IO3_AF(void);
  uint8_t AutoPollingMemReady(uint32_t Timeout);
  uint8_t AutoPollingMemReady_IT(void);
  uint8_t EnterMemory_QPI(void);
  void init_command(QSPI_CommandTypeDef *s_command);

  uint8_t done_TXing(void) { return status == STATUS_TX_COMPLETE; }

  static uint8_t test_encode_num(uint32_t num)	{return (num*7) + (num>>7);}

public:

  uint8_t is_ready(void) { return status == STATUS_READY; }

  uint8_t Test(void);
  uint8_t Test_Sector(uint8_t sector_num);

  uint32_t get_64kblock_addr(uint8_t block64k_num);
  uint32_t get_32kblock_addr(uint8_t block32k_num);
  uint32_t get_sector_addr(uint8_t sector_num);

  uint8_t Init(void);
  uint8_t Reset(void);

  uint8_t Read(uint8_t* pData, uint32_t read_addr, uint32_t num_bytes, UseInterruptFlags use_interrupt);
  uint8_t Write(uint8_t* pData, uint32_t write_addr, uint32_t num_bytes);
  uint8_t Write_Page(uint8_t* pData, uint32_t write_addr, uint32_t num_bytes, UseInterruptFlags use_interrupt);

  uint8_t Erase(ErasableSizes size, uint32_t BaseAddress, UseInterruptFlags use_interrupt);


  static QSpiFlash *instance_;

};
