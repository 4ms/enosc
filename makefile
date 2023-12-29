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
	src/drivers/dac.cc \
	src/drivers/spi_adc.cc \
	src/drivers/leds.cc \
	src/drivers/buttons.cc \
	src/drivers/debug.cc \
	src/drivers/qspi_flash.cc \
	src/dynamic_data.cc \
	src/main.cc \

OBJS_1 = $(SRCS:.cc=.o)
OBJS = $(OBJS_1:.c=.o)

TEST_SRCS = test/test.cc data.cc lib/easiglib/numtypes.cc lib/easiglib/math.cc lib/easiglib/dsp.cc src/dynamic_data.cc

DEPS = $(addsuffix .d, $(SRCS)) $(addsuffix .d, $(TEST_SRCS))

TEST_OBJS = $(TEST_SRCS:.cc=.test.o)

HAL = 	stm32f7xx_hal.o \
	stm32f7xx_hal_cortex.o \
	stm32f7xx_hal_gpio.o \
	stm32f7xx_hal_rcc.o \
	stm32f7xx_hal_rcc_ex.o \
	stm32f7xx_hal_dma.o \
	stm32f7xx_hal_sai.o \
	stm32f7xx_hal_sai_ex.o \
	stm32f7xx_hal_spi.o \
	stm32f7xx_hal_adc.o \
	stm32f7xx_hal_adc_ex.o \
	stm32f7xx_hal_pwr.o \
	stm32f7xx_hal_pwr_ex.o \
	stm32f7xx_hal_qspi.o \
	stm32f7xx_hal_tim.o \
	stm32f7xx_hal_tim_ex.o \

OPTIM ?= 2
TOOLCHAIN_DIR ?=

CXX = $(TOOLCHAIN_DIR)arm-none-eabi-g++
CC = $(TOOLCHAIN_DIR)arm-none-eabi-gcc
OBJCOPY = $(TOOLCHAIN_DIR)arm-none-eabi-objcopy
GDB = $(TOOLCHAIN_DIR)arm-none-eabi-gdb
CMDSIZE = $(TOOLCHAIN_DIR)arm-none-eabi-size

TEST_CXX = g++-12

CMSIS_DIR = lib/CMSIS/
HAL_DIR = lib/HAL/
EASIGLIB_DIR = lib/easiglib/

INC = -I . \
      -I src/ \
      -I src/drivers \
      -I $(EASIGLIB_DIR) \
      -I $(CMSIS_DIR) \
      -I $(HAL_DIR) \

LDSCRIPT = $(CMSIS_DIR)STM32F730V8x_FLASH.ld

ARCHFLAGS =  	-mcpu=cortex-m7 \
		-mthumb \
		-mfloat-abi=hard \
		-mfpu=fpv5-d16 \
		-mthumb-interwork \
		-mfp16-format=ieee \
		-DARM_MATH_CM7 \
		-DSTM32F730xx \

CPPFLAGS= $(INC)

CFLAGS= $(ARCHFLAGS) \
	-g \
	-ffast-math \
	-fdata-sections \
	-ffunction-sections \
	-ffreestanding \
	--param l1-cache-size=8 \
	--param l1-cache-line-size=32 \
	-DUSE_HAL_DRIVER \
	--specs=nano.specs
#	-DUSE_FULL_ASSERT \
#       -DTEST \

CXXFLAGS=$(CFLAGS) \
	-std=c++17 \
	-fno-rtti \
	-fno-exceptions \
	-Werror=return-type \
	-Wdouble-promotion \
	-Wno-register \

LDFLAGS= $(CXXFLAGS) -T $(LDSCRIPT) \
	-Wl,--gc-sections -Wl,-Map,main.map \

STARTUP = $(CMSIS_DIR)startup_stm32f730xx
SYSTEM = $(CMSIS_DIR)system_stm32f7xx

OBJS := $(STARTUP).o \
	$(SYSTEM).o \
	$(addprefix $(HAL_DIR), $(HAL)) \
	$(OBJS)

OPTFLAG= -O$(OPTIM)

$(addprefix $(HAL_DIR), $(HAL)): OPTFLAG= -Os

all: $(TARGET).hex $(TARGET).bin

%.elf: data.hh $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
	$(CMDSIZE) $<

%.hex: %.elf
	$(OBJCOPY) --output-target=ihex $< $@

%.o: %.s
	$(CC) -c -x assembler-with-cpp $(ASFLAGS) $< -o $@

combo:
	cd bootloader && $(MAKE)

bootloader: combo

data.cc data.hh: $(EASIGLIB_DIR)data_compiler.py data/data.py
	PYTHONPATH=$(EASIGLIB_DIR) python3 data/data.py

clean:
	rm -f $(OBJS) $(TEST_OBJS) $(DEPS) $(TARGET).elf $(TARGET).bin $(TARGET).hex  \ 
	main.map test/test $(EASIGLIB_DIR)data_compiler.pyc

realclean: clean
	rm data.cc data.hh 

bootloader-clean: clean
	rm -rf bootloader/build
	

flash: $(TARGET).bin
	st-flash write $(TARGET).bin 0x8004000

erase:
	st-flash erase

debug-server:
	openocd -f interface/stlink-v2-1.cfg -f target/stm32f7x.cfg

debug:
	gdb $(TARGET).elf \
	-x gdbinit

# File dependencies:

DEPFLAGS = -MMD -MP -MF $<.d

%.o: %.c
	$(CC) $(CFLAGS) $(OPTFLAG) $(CPPFLAGS) -c $< -o $@

%.o: %.cc %.cc.d
	$(CXX) $(DEPFLAGS) $(OPTFLAG) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

%.d: ;

# Test build:

TEST_CXXFLAGS= \
	-std=c++17 \
	-fno-rtti \
	-fno-exceptions \
	-Werror=return-type \
	-Wdouble-promotion \
	-Wno-register \
	-g \
	-ffast-math \
	-O2

test: test/test

test/test: data.hh test/test.cc $(TEST_OBJS)
	$(TEST_CXX) -o $@ $(CPPFLAGS) $(TEST_CXXFLAGS) $(TEST_OBJS) $(LIBS)

%.test.o: %.cc %.cc.d
	$(TEST_CXX) $(DEPFLAGS) $(CPPFLAGS) $(TEST_CXXFLAGS) -DTEST -c $< -o $@

wav: fsk-wav

fsk-wav: $(TARGET).bin
	PYTHONPATH='bootloader/:.' && python3 bootloader/stm_audio_bootloader/fsk/encoder.py \
		-s 22050 -b 16 -n 8 -z 4 -p 256 -g 16384 -k 1800 \
		$(TARGET).bin


-include $(DEPS)

.PRECIOUS: $(DEPS) $(OBJS) $(TEST_OBJS) $(TARGET).elf data.cc data.hh
.PHONY: all clean flash erase debug debug-server
