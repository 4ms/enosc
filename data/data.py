#!/usr/bin/env python

import numpy as np

data = {}

# Pitch to frequency conversion

size = 256
ratios_low = 2.0 ** (np.arange(-size/2.0,size/2.0)/12.0)
data['pitch_ratios_low'] = ratios_low

size = 128
ratios_high = 2.0 ** (np.linspace(0.,1.,num=128))
data['pitch_ratios_high'] = ratios_high

# sine

size = 1024
spc = np.arange(0., size+1)/size
sine = np.sin(spc*2*np.pi)
data['sine'] = sine

data['short_sine'] = (sine*32767.0).astype(np.int16)

# Generate

import data_compiler
data_compiler.compile(data)
