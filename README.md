# Ensemble Oscillator

### A Eurorack module from 4ms Company and Matthias Puech

More information on the [4ms website](https://4mscompany.com/enosc)

Requires arm-none-eabi-gcc v8 or later, and python 3 (for generating data.hh and data.cc, and optional .wav file for use with the audio bootloader)

To build the .elf file (and .hex and .bin):
```
make
```

To build .wav file for use with audio bootloader:
```
make wav
```

To flash with an st-link:
```
make flash
```

To build the audio bootloader and generate a "combo" hex file (containing bootloader and app in one hex file):
```
make bootloader
```

Note: building the bootloader is not normally required, as it is already present on all Ensemble Oscillator units.

Note: The source code is built for the STM32F730 chip, but will run without modification on the STM32F722, STM32F765, and STM32F767 chips.
