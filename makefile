# Simple makefile template for STM32 project
#
# - include and list objects in OBJS
# - define TARGET

TARGET = main
SRCS = \
	data.cc \
	lib/easiglib/numtypes.cc \
	lib/easiglib/math.cc \
	lib/easiglib/dsp.cc \
	src/drivers/adc.cc \
	src/drivers/codec.cc \
	src/main.cc \

OBJS_1 = $(SRCS:.cc=.o)
OBJS = $(OBJS_1:.c=.o)

TEST_SRCS = test/test.cc data.cc lib/easiglib/numtypes.cc lib/easiglib/math.cc lib/easiglib/dsp.cc

DEPS = $(addsuffix .d, $(SRCS)) $(addsuffix .d, $(TEST_SRCS))

TEST_OBJS = $(TEST_SRCS:.cc=.test.o)

HAL = 	stm32f7xx_hal.o \
	stm32f7xx_hal_cortex.o \
	stm32f7xx_hal_gpio.o \
	stm32f7xx_hal_rcc.o \
	stm32f7xx_hal_rcc_ex.o \
	stm32f7xx_hal_dma.o \
	stm32f7xx_hal_i2c.o \
	stm32f7xx_hal_i2c_ex.o \
	stm32f7xx_hal_sai.o \
	stm32f7xx_hal_sai_ex.o \
	stm32f7xx_hal_spi.o \
	stm32f7xx_hal_adc.o \
	stm32f7xx_hal_adc_ex.o \
	stm32f7xx_hal_pwr.o \
	stm32f7xx_hal_pwr_ex.o \
	stm32f7xx_hal_tim.o \
	stm32f7xx_hal_tim_ex.o \

OPTIM ?= 2
TOOLCHAIN_DIR ?=

CXX = $(TOOLCHAIN_DIR)arm-none-eabi-g++
CC = $(TOOLCHAIN_DIR)arm-none-eabi-gcc
OBJCOPY = $(TOOLCHAIN_DIR)arm-none-eabi-objcopy
GDB = $(TOOLCHAIN_DIR)arm-none-eabi-gdb

TEST_CXX = g++-8

CMSIS_DIR = lib/CMSIS/
HAL_DIR = lib/HAL/
EASIGLIB_DIR = lib/easiglib/

INC = -I . \
      -I src/ \
      -I src/drivers \
      -I $(EASIGLIB_DIR) \
      -I $(CMSIS_DIR) \
      -I $(HAL_DIR) \

LDSCRIPT = $(CMSIS_DIR)/STM32F722VEx_FLASH.ld

ARCHFLAGS =  	-mcpu=cortex-m7 \
		-mthumb \
		-mfloat-abi=hard \
		-mfpu=fpv5-d16 \
		-mthumb-interwork \
		-mfp16-format=ieee \
		-DARM_MATH_CM7 \
		-DSTM32F722xx \

CPPFLAGS= $(INC)

CFLAGS= $(ARCHFLAGS) \
	-g \
	-O$(OPTIM) \
	-DUSE_HAL_DRIVER \
	-DUSE_FULL_ASSERT \
	-ffast-math \
	-fdata-sections \
	-ffunction-sections \
	-ffreestanding \
#       -DTEST \

CXXFLAGS=$(CFLAGS) \
	-std=c++14 \
	-fno-rtti \
	-fno-exceptions \
	-Wfatal-errors \
	-Werror=return-type \
	-Wdouble-promotion \

LDFLAGS= $(ARCHFLAGS) -T $(LDSCRIPT) \
	-Wl,--gc-sections \
	-nostdlib \

STARTUP = $(CMSIS_DIR)startup_stm32f722xx
SYSTEM = $(CMSIS_DIR)system_stm32f7xx

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
	st-flash write $(TARGET).bin 0x8000000

erase:
	st-flash erase

debug-server:
	st-util -v

debug:
	$(TOOLCHAIN_DIR)arm-none-eabi-gdb $(TARGET).elf \
	--eval-command="target remote localhost:4242"

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
