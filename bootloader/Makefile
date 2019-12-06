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


ARCH_CFLAGS = -DARM_MATH_CM7 \
 	-D'__FPU_PRESENT=1' \
  	-DUSE_FULL_LL_DRIVER \
  	-DSTM32F730xx \
  	-DF_CPU=$(F_CPU) \

# F_FLTO = -flto

OPTFLAG = -Os
OPT0  = -O0

CFLAGS  = -g3 \
		$(ARCH_CFLAGS) $(MCU) \
		-I.  $(INCLUDES) \
		-fdata-sections -ffunction-sections \
	   	-ffreestanding \
 		$(F_FLTO) \


CXXFLAGS=$(CFLAGS) \
	-std=c++17 \
	-fno-rtti \
	-fno-exceptions \
	-Werror=return-type \
	-Wdouble-promotion \
	-Wno-register \

AFLAGS = -mcpu=cortex-m7 \
	-mthumb-interwork \
	-mlittle-endian \
 	-mfpu=fpv5-d16 \
 	-mfloat-abi=hard \

LDSCRIPT = $(LOADFILE)

LFLAGS  = $(MCU) -specs=nano.specs  -T $(LDSCRIPT) \
	-Wl,--gc-sections \
	$(F_FLTO)
	
# $(BLBUILDDIR)/bootloader.o: OPTFLAG = $(OPT0)
# $(BLBUILDDIR)/hardware_test.o: OPTFLAG = $(OPT0)
# $(BLBUILDDIR)/dac_sai.o: OPTFLAG = $(OPT0)
# $(BLBUILDDIR)/qspi_flash_ll.o: OPTFLAG = $(OPT0)
# $(BLBUILDDIR)/qspi_flash.o: OPTFLAG = $(OPT0)
# $(BLBUILDDIR)/adc_spi.o: OPTFLAG = $(OPT0)
# $(BLBUILDDIR)/bl_ui.o: OPTFLAG = $(OPT0)
# $(BLBUILDDIR)/stm_audio_bootloader/fsk/packet_decoder.o: OPTFLAG = $(OPT0)

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
	$(CC) -c $(OPTFLAG) $(CFLAGS) $< -o $@


$(BLBUILDDIR)/%.o: %.cc
	mkdir -p $(dir $@)
	$(CPP) -c $(OPTFLAG) $(CFLAGS) $(CXXFLAGS) $< -o $@


$(BLBUILDDIR)/%.o: %.s
	mkdir -p $(dir $@)
	$(AS) $(AFLAGS) $< -o $@ > $(addprefix $(BLBUILDDIR)/, $(addsuffix .lst, $(basename $<)))


flash: $(BIN)
	st-flash write $(BIN) 0x8000000

clean:
	rm -rf $(BUILDDIR)

.PHONY: all combo

	