# Makefile by Dan Green <danngreen1@gmail.com>
#

BINARYNAME = bootloader
MAINAPP_HEX = ../main.hex

APP_SOURCES = 	\
	../src/drivers/leds.cc \
	../src/drivers/buttons.cc \
	../src/drivers/debug.cc \


BUILDDIR = build
BLBUILDDIR = $(BUILDDIR)/$(BINARYNAME)
COMBODIR = $(BUILDDIR)/bootloader-app-combo
COMBO = $(COMBODIR)/bootloader-app-combo

CMSIS_DIR = ../lib/CMSIS
HAL_DIR = ../lib/HAL
EASIGLIB_DIR = ../lib/easiglib/

STARTUP = $(CMSIS_DIR)/startup_stm32f730xx
SYSTEM = $(CMSIS_DIR)/system_stm32f7xx

LOADFILE = bootloader_STM32F730V8x.ld

SOURCES += $(wildcard *.c)
SOURCES += $(wildcard *.cc)
SOURCES += stm_audio_bootloader/fsk/packet_decoder.cc
SOURCES += $(APP_SOURCES)
SOURCES += $(wildcard $(HAL_DIR)/*.c)
SOURCES += $(STARTUP).s
SOURCES += $(SYSTEM).c



OBJECTS = $(addprefix $(BLBUILDDIR)/, $(addsuffix .o, $(basename $(SOURCES))))

INCLUDES += -I$(CMSIS_DIR) \
			-I$(HAL_DIR) \
			-I$(EASIGLIB_DIR) \
			-I stm_audio_bootloader/fsk \
			-I stmlib \
			-I stmlib/utils \
			-I ../src \
			-I ../src/drivers \
			-I ../



ELF = $(BLBUILDDIR)/$(BINARYNAME).elf
HEX = $(BLBUILDDIR)/$(BINARYNAME).hex
BIN = $(BLBUILDDIR)/$(BINARYNAME).bin

ARCH = arm-none-eabi
CC = $(ARCH)-gcc
CPP = $(ARCH)-g++
LD = $(ARCH)-gcc -Wl,-Map,build/main.map,--cref -Wl,--gc-sections
AS = $(ARCH)-as
OBJCPY = $(ARCH)-objcopy
OBJDMP = $(ARCH)-objdump
CMDSIZE = $(ARCH)-size
GDB = $(ARCH)-gdb


MCU = -mcpu=cortex-m7 \
	-mthumb-interwork \
	-mlittle-endian \
	-mfp16-format=ieee \
 	-mfpu=fpv5-d16 \
 	-mfloat-abi=hard

F_CPU = 216000000L

ARCH_CFLAGS = -D'IS_BOOTLOADER=1' -DARM_MATH_CM7 \
 	-D'__FPU_PRESENT=1' \
  	-DUSE_HAL_DRIVER \
  	-DSTM32F730xx \
  	-DF_CPU=$(F_CPU) \


OPTIMIZED_CFLAGS = -g2 -Os

CFLAGS  = $(OPTIMIZED_CFLAGS) 
CFLAGS += $(ARCH_CFLAGS) $(MCU)
CFLAGS += -I.  $(INCLUDES)
CFLAGS += -fdata-sections -ffunction-sections
# CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"

C0FLAGS  = -O0 -g2 -Wall
C0FLAGS += $(ARCH_CFLAGS) $(MCU)
C0FLAGS += -I.  $(INCLUDES)

CXXFLAGS=$(CFLAGS) \
	-std=c++17 \
	-fno-rtti \
	-fno-exceptions \
	-Werror=return-type \
	-Wdouble-promotion \
	-Wno-register \

AFLAGS = $(MCU)

LDSCRIPT = $(LOADFILE)

LFLAGS  = $(MCU) -specs=nano.specs  -T $(LDSCRIPT) -lc -lm -lnosys
	
# $(BLBUILDDIR)/bootloader.o: CFLAGS = $(C0FLAGS)
# $(BLBUILDDIR)/bl_ui.o: CFLAGS = $(C0FLAGS)
# $(BLBUILDDIR)/stm_audio_bootloader/fsk/packet_decoder.o: CFLAGS = $(C0FLAGS)

combo: $(COMBO).bin 

$(COMBO).bin:  $(MAINAPP_HEX) $(HEX)
	mkdir -p $(COMBODIR)
	cat  $(MAINAPP_HEX) $(HEX) | \
	awk -f util/merge_hex.awk > $(COMBO).hex
	$(OBJCPY) -I ihex -O binary $(COMBO).hex $(COMBO).bin

all: Makefile $(BIN) $(HEX)

$(BIN): $(ELF)
	$(OBJCPY) -O binary $< $@
	$(OBJDMP) -x --syms $< > $(addsuffix .dmp, $(basename $<))
	$(CMDSIZE) $<
	ls -l $@ $<

$(HEX): $(ELF)
	$(OBJCPY) --output-target=ihex $< $@

$(ELF): $(OBJECTS) 
	$(LD) $(LFLAGS) -o $@ $(OBJECTS)


$(BLBUILDDIR)/%.o: %.c $(wildcard inc/*.h) $(wildcard inc/res/*.h)
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@


$(BLBUILDDIR)/%.o: %.cc $(wildcard inc/*.h) $(wildcard inc/res/*.h)
	mkdir -p $(dir $@)
	$(CPP) -c $(CFLAGS) $(CXXFLAGS) $< -o $@


$(BLBUILDDIR)/%.o: %.s
	mkdir -p $(dir $@)
	$(AS) $(AFLAGS) $< -o $@ > $(addprefix $(BLBUILDDIR)/, $(addsuffix .lst, $(basename $<)))


flash: $(BIN)
	st-flash write $(BIN) 0x8000000

clean:
	rm -rf $(BUILDDIR)
	