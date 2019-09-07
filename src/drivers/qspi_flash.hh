#pragma once

#define QSPI_FLASH_SIZE_BYTES            0x40000 	// 256 KBytes
#define QSPI_64KBLOCK_SIZE               0x10000  	// 64 KBytes, hence the name "64K Block" :)
#define QSPI_32KBLOCK_SIZE               0x8000   	// 32 KBytes, hence the name "32K Block" :)
#define QSPI_SECTOR_SIZE                 0x1000   	// 4 KBytes sectors
#define QSPI_PAGE_SIZE                   0x100    	// 256 Byte pages
#define QSPI_PAGE_ADDRESS_BITS           8    		// 8 bits = 256 addresses per page

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

  HAL_StatusTypeDef WriteEnable(void);
  void GPIO_Init_1IO(void);
  void GPIO_Init_IO2_IO3_AF(void);
  HAL_StatusTypeDef AutoPollingMemReady(uint32_t Timeout);
  HAL_StatusTypeDef AutoPollingMemReady_IT(void);
  HAL_StatusTypeDef EnterMemory_QPI(void);
  void init_command(QSPI_CommandTypeDef *s_command);

  bool done_TXing(void) { return status == STATUS_TX_COMPLETE; }

  uint8_t test_encode_num(uint32_t num)	{return (num*7) + (num>>7);}

public:
  QSpiFlash();

  bool is_ready(void) { return status == STATUS_READY; }

  bool Test(void);
  bool Test_Sector(uint8_t sector_num);

  static uint32_t get_64kblock_addr(uint8_t block64k_num);
  static uint32_t get_32kblock_addr(uint8_t block32k_num);
  static uint32_t get_sector_addr(uint8_t sector_num);

  HAL_StatusTypeDef Reset(void);

  bool Read(uint8_t* pData, uint32_t read_addr, uint32_t num_bytes,
               UseInterruptFlags use_interrupt);
  bool Write(uint8_t* pData, uint32_t write_addr, uint32_t num_bytes);
  bool Write_Page(uint8_t* pData, uint32_t write_addr, uint32_t num_bytes,
                  UseInterruptFlags use_interrupt);

  bool Erase(ErasableSizes size, uint32_t BaseAddress, UseInterruptFlags use_interrupt);

  static QSpiFlash *instance_;

};

template<int block_nr>
struct FlashBlock {
  static constexpr int size_ = QSPI_32KBLOCK_SIZE;

  static bool Read(uint8_t *data, int size) {
    while(!QSpiFlash::instance_->is_ready());
    return QSpiFlash::instance_->Read(data, QSpiFlash::get_32kblock_addr(block_nr),
                                      size, QSpiFlash::EXECUTE_BACKGROUND);
  }

  static bool Write(uint8_t *data, int size) {
    while(!QSpiFlash::instance_->is_ready());
    return QSpiFlash::instance_->Write_Page(data, QSpiFlash::get_32kblock_addr(block_nr),
                                            size, QSpiFlash::EXECUTE_BACKGROUND);
  }
};
