# Makefile by Dan Green <danngreen1@gmail.com>
#

BINARYNAME = bootloader
MAINAPP_HEX = ../main.hex

BUILDDIR = build
BLBUILDDIR = $(BUILDDIR)/$(BINARYNAME)
COMBODIR = $(BUILDDIR)/bootloader-app-combo
COMBO = $(COMBODIR)/bootloader-app-combo

CMSIS_DIR = ../lib/CMSIS
LL_DIR = lib

STARTUP = $(CMSIS_DIR)/startup_stm32f730xx
SYSTEM = $(CMSIS_DIR)/system_stm32f7xx

LOADFILE = bootloader_STM32F730V8x.ld

SOURCES += $(wildcard *.c)
SOURCES += $(wildcard *.cc)
SOURCES += $(wildcard $(LL_DIR)/*.c)
SOURCES += stm_audio_bootloader/fsk/packet_decoder.cc
SOURCES += $(STARTUP).s
SOURCES += $(SYSTEM).c

OBJECTS = $(addprefix $(BLBUILDDIR)/, $(addsuffix .o, $(basename $(SOURCES))))

INCLUDES += -I$(CMSIS_DIR) \
			-I $(LL_DIR) \
			-I stm_audio_bootloader/fsk \
			-I stmlib \
			-I stmlib/utils \
			-I ../src \

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

# F_FLTO = -flto

ARCH_CFLAGS = -DARM_MATH_CM7 \
 	-D'__FPU_PRESENT=1' \
  	-DUSE_FULL_LL_DRIVER \
  	-DSTM32F730xx \
  	-DF_CPU=$(F_CPU) \


OPTIMIZED_CFLAGS = -g3 -Os

CFLAGS  = $(OPTIMIZED_CFLAGS) 
CFLAGS += $(ARCH_CFLAGS) $(MCU)
CFLAGS += -I.  $(INCLUDES)
CFLAGS += -fdata-sections -ffunction-sections \
 	$(F_FLTO)
# CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)"

C0FLAGS  = -O0 -g3 -Wall
C0FLAGS += $(ARCH_CFLAGS) $(MCU)
C0FLAGS += -I.  $(INCLUDES)

CXXFLAGS=$(CFLAGS) \
	-std=c++17 \
	-fno-rtti \
	-fno-exceptions \
	-Werror=return-type \
	-Wdouble-promotion \
	-Wno-register \
	$(F_FLTO)

AFLAGS = -mcpu=cortex-m7 \
	-mthumb-interwork \
	-mlittle-endian \
 	-mfpu=fpv5-d16 \
 	-mfloat-abi=hard

LDSCRIPT = $(LOADFILE)

LFLAGS  = $(MCU) -specs=nano.specs  -T $(LDSCRIPT) \
	-Wl,--gc-sections \
	$(F_FLTO)
	
# $(BLBUILDDIR)/bootloader.o: CFLAGS = $(C0FLAGS)
# $(BLBUILDDIR)/hardware_test.o: CFLAGS = $(C0FLAGS)
# $(BLBUILDDIR)/dac_sai.o: CFLAGS = $(C0FLAGS)
# $(BLBUILDDIR)/adc_spi.o: CFLAGS = $(C0FLAGS)
# $(BLBUILDDIR)/bl_ui.o: CFLAGS = $(C0FLAGS)
# $(BLBUILDDIR)/stm_audio_bootloader/fsk/packet_decoder.o: CFLAGS = $(C0FLAGS)

all: Makefile $(BIN) $(HEX) $(COMBO).bin

combo: $(COMBO).bin 

$(COMBO).bin:  $(MAINAPP_HEX) $(HEX)
	mkdir -p $(COMBODIR)
	cat  $(MAINAPP_HEX) $(HEX) | \
	awk -f util/merge_hex.awk > $(COMBO).hex
	$(OBJCPY) -I ihex -O binary $(COMBO).hex $(COMBO).bin


$(BIN): $(ELF)
	$(OBJCPY) -O binary $< $@
	$(OBJDMP) -x --syms $< > $(addsuffix .dmp, $(basename $<))
	$(CMDSIZE) $<

$(HEX): $(ELF)
	$(OBJCPY) --output-target=ihex $< $@

$(ELF): $(OBJECTS) 
	$(LD) $(LFLAGS) -o $@ $(OBJECTS)


$(BLBUILDDIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@


$(BLBUILDDIR)/%.o: %.cc
	mkdir -p $(dir $@)
	$(CPP) -c $(CFLAGS) $(CXXFLAGS) $< -o $@


$(BLBUILDDIR)/%.o: %.s
	mkdir -p $(dir $@)
	$(AS) $(AFLAGS) $< -o $@ > $(addprefix $(BLBUILDDIR)/, $(addsuffix .lst, $(basename $<)))


flash: $(BIN)
	st-flash write $(BIN) 0x8000000

clean:
	rm -rf $(BUILDDIR)

.PHONY: all combo

	