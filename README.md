# Ensemble Oscillator

## A Eurorack module from 4ms Company and Matthias Puech

More information on the [4ms website](https://4mscompany.com/enosc)

### Installing requirements

Requires arm-none-eabi-gcc v8 or later, and python 3 (for generating data.hh and data.cc, and optional .wav file for use with the audio bootloader).

You can download arm-none-eabi-gcc from ARM's website [here](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads). 
Choose the package for your computer called "AArch32 bare-metal target (arm-none-eabi)".

If you install via a package manager, you may need to also install the `arm-none-eabi-newlib` package as well.

For the python scripts, you need `numpy` installed:
```
# On some systems:
pip3 install numpy

# Or on some systems:
pip install numpy
```

### Building

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
