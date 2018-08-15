# Simple makefile template for STM32 project
#
# - include and list objects in OBJS
# - define TARGET

TARGET = main
OBJS = lib/aesig/numtypes.o src/main.o data.o
HAL = 	stm32f4xx_hal.o \
	stm32f4xx_hal_cortex.o \
	stm32f4xx_hal_gpio.o \
	stm32f4xx_hal_rcc.o \
	stm32f4xx_hal_rcc_ex.o \
	stm32f4xx_hal_dma.o \
	stm32f4xx_hal_i2c.o \
	stm32f4xx_hal_i2s.o \
	stm32f4xx_hal_i2s_ex.o \
	stm32f4xx_hal_spi.o \
	stm32f4xx_hal_dac.o \
	stm32f4xx_hal_adc.o \
	stm32f4xx_hal_adc_ex.o \
	stm32f4xx_hal_rng.o \

OPTIM ?= 2
TOOLCHAIN_DIR ?=

CXX = $(TOOLCHAIN_DIR)arm-none-eabi-g++
CC = $(TOOLCHAIN_DIR)arm-none-eabi-gcc
OBJCOPY = $(TOOLCHAIN_DIR)arm-none-eabi-objcopy
GDB = $(TOOLCHAIN_DIR)arm-none-eabi-gdb

CMSIS_DIR = lib/CMSIS/
HAL_DIR = lib/HAL/
AESIG_DIR = lib/aesig/

INC =   -I src/ \
	-I . \
	-I $(AESIG_DIR) \
	-I $(CMSIS_DIR) \
	-I $(HAL_DIR) \

LDSCRIPT = $(CMSIS_DIR)/STM32F407VGTx_FLASH.ld

ARCHFLAGS =  	-mcpu=cortex-m4 \
		-mthumb \
		-mfloat-abi=hard \
		-mfpu=fpv4-sp-d16 \
		-mthumb-interwork \
		-mfp16-format=ieee \
		-DSTM32F407xx \

CPPFLAGS= $(INC)

CFLAGS= $(ARCHFLAGS) \
	-g \
	-O$(OPTIM) \
	-DUSE_HAL_DRIVER \
	-DUSE_FULL_ASSERT \
	-fdata-sections \
	-ffunction-sections \

CXXFLAGS=$(CFLAGS) \
	-std=c++14 \
	-fno-rtti \
	-fno-exceptions \
	-Werror=return-type \
	-Wdouble-promotion \

LDFLAGS= $(ARCHFLAGS) -T $(LDSCRIPT) \
	-Wl,--gc-sections \
	-nostartfiles \
	-nostdlib \

STARTUP = $(CMSIS_DIR)startup_stm32f407xx
SYSTEM = $(CMSIS_DIR)system_stm32f4xx

OBJS += $(STARTUP).o \
	$(SYSTEM).o \
	$(addprefix $(HAL_DIR), $(HAL)) \

all: $(TARGET).bin

%.elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

%.o: %.s
	$(CC) -c -x assembler-with-cpp $(ASFLAGS) $< -o $@

data.cc data.hh: $(AESIG_DIR)/data_compiler.py data/data.py
	PYTHONPATH=$(AESIG_DIR) python data/data.py

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin data.cc data.hh \
	$(AESIG_DIR)/data_compiler.pyc

flash: $(TARGET).bin
	openocd -f interface/stlink-v2-1.cfg -f target/stm32f4x.cfg \
	-c "init; program $(TARGET).bin verify reset exit 0x08000000" \

erase:
	openocd -f interface/stlink-v2-1.cfg -f target/stm32f4x.cfg \
	-c "init; halt; stm32f4x mass_erase 0; exit" \

debug-server:
	openocd -f interface/stlink-v2-1.cfg -f target/stm32f4x.cfg \

debug:
	$(TOOLCHAIN_DIR)arm-none-eabi-gdb $(TARGET).elf \
	--eval-command="target remote localhost:3333"

.PRECIOUS: $(OBJS) $(TARGET).elf data.cc data.hh

# File dependencies:
src/main.o: $(AESIG_DIR)/numtypes.hh \
	    src/parameters.hh \
	    src/drivers/leds.hh src/drivers/dac.hh src/drivers/button.hh \
	    src/drivers/system.hh src/drivers/debug_pins.hh \
	    src/drivers/accelerometer.hh src/drivers/adc.hh src/drivers/rng.hh \
	    src/oscillator.hh \
	    src/ui.hh \
	    data.hh

src/oscillator.hh: ${AESIG_DIR}/dsp.hh

$(AESIG_DIR)/numtypes.hh:
$(AESIG_DIR)/dsp.hh: data.hh $(AESIG_DIR)/numtypes.hh $(AESIG_DIR)/filter.hh
$(AESIG_DIR)/buffer.hh: $(AESIG_DIR)/util.hh $(AESIG_DIR)/numtypes.hh

data.hh: $(AESIG_DIR)/buffer.hh

src/*.o: src/*.hh
