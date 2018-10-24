# Simple makefile template for STM32 project
#
# - include and list objects in OBJS
# - define TARGET

TARGET = main
SRCS = lib/easiglib/numtypes.cc lib/easiglib/math.cc lib/easiglib/dsp.cc src/main.cc data.cc
OBJS = $(SRCS:.cc=.o)

TEST_SRCS = test/test.cc data.cc lib/easiglib/numtypes.cc lib/easiglib/math.cc lib/easiglib/dsp.cc

DEPS = $(addsuffix .d, $(SRCS)) $(addsuffix .d, $(TEST_SRCS))

TEST_OBJS = $(TEST_SRCS:.cc=.test.o)

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

TEST_CXX = x86_64-apple-darwin17.7.0-c++-8

CMSIS_DIR = lib/CMSIS/
HAL_DIR = lib/HAL/
EASIGLIB_DIR = lib/easiglib/

INC = -I . \
      -I src/ \
      -I $(EASIGLIB_DIR) \
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
	-ffreestanding \

CXXFLAGS=$(CFLAGS) \
	-std=c++14 \
	-fno-rtti \
	-fno-exceptions \
	-Werror=return-type \
	-Wdouble-promotion \

LDFLAGS= $(ARCHFLAGS) -T $(LDSCRIPT) \
	-Wl,--gc-sections \
	-nostdlib \

STARTUP = $(CMSIS_DIR)startup_stm32f407xx
SYSTEM = $(CMSIS_DIR)system_stm32f4xx

OBJS += $(STARTUP).o \
	$(SYSTEM).o \
	$(addprefix $(HAL_DIR), $(HAL)) \

all: $(TARGET).bin

%.elf: data.hh $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

%.o: %.s
	$(CC) -c -x assembler-with-cpp $(ASFLAGS) $< -o $@

data.cc data.hh: $(EASIGLIB_DIR)data_compiler.py data/data.py
	PYTHONPATH=$(EASIGLIB_DIR) python data/data.py

clean:
	rm -f $(OBJS) $(TEST_OBJS) $(DEPS) $(TARGET).elf $(TARGET).bin test/test data.cc data.hh \
	$(EASIGLIB_DIR)data_compiler.pyc

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

# File dependencies:

DEPFLAGS = -MMD -MP -MF $<.d

%.o: %.c %.c.d
	$(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

%.o: %.cc %.cc.d
	$(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

%.d: ;

# Test build:

test: test/test

test/test: data.hh test/test.cc $(TEST_OBJS)
	$(TEST_CXX) -o $@ $(TEST_OBJS) $(LIBS)

%.test.o: %.cc %.cc.d
	$(TEST_CXX) $(DEPFLAGS) $(CPPFLAGS) -c $< -o $@

include $(DEPS)

.PRECIOUS: $(DEPS) $(OBJS) $(TEST_OBJS) $(TARGET).elf data.cc data.hh
.PHONY: all clean flash erase debug debug-server
