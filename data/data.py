#!/usr/bin/env python

import numpy as np

data = {}

# Pitch to frequency conversion

size = 256
ratios_low = 2.0 ** (np.arange(-size/2.0,size/2.0)/12.0)
data['pitch_ratios_high'] = ratios_low

size = 256
ratios_high = 2.0 ** (np.linspace(0.,1./12.,num=size))
data['pitch_ratios_low'] = ratios_high

# sine

size = 1024
spc = np.arange(0., size+1)/size
sine = np.sin(spc*2*np.pi)
data['sine'] = sine

data['short_sine'] = (sine*32767.0).astype(np.int16)

# base-2 exponential

size = 1024
spc = np.arange(0., size) / size
exp2 = 2 ** spc

data['exp2_u0_23'] = (exp2*(2**23)).astype(np.uint32)

# normalization factors for sums of uncorrelated signals

# we compute the pdfs of the repeated sums of uniform distributions
# [-1..1], and for each take the value at which they pass a certain threshold
# of "acceptable distortion probability"

size = 127
resolution = 512
threshold = 0.00001

u = np.ones(resolution)
v = u
factors = []

for i in range(size):
    # plt.plot(u)
    half = u[len(u)/2:]
    half = np.append(half, 0.)
    factor = np.argmax(half<threshold) / float(resolution/2)
    factors.append(1.0 / factor)
    u = np.convolve(u, v) / resolution

data['normalization_factors'] = factors

# Generate

import data_compiler
data_compiler.compile("data", "Data", data);
