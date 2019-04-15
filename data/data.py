#!/usr/bin/env python

import numpy as np
import matplotlib.pyplot as plt

from data_compiler import *

data = {}

# sine

size = 256
spc = np.arange(0., size+1)/size
sine = np.sin(spc*2*np.pi)

data['sine'] = [s1_15(i) for i in sine]

# base-2 exponential

size = 128
spc = np.arange(0., size) / size
exp2 = 2 ** spc

data['exp2_u0_23'] = (exp2*(2**23)).astype(np.uint32)

# normalization factors for sums of uncorrelated signals

# we compute the pdfs of the repeated sums of uniform distributions
# [-1..1], and for each take the value at which they pass a certain threshold
# of "acceptable distortion probability"

size = 32
resolution = 512
threshold = 0.000001

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

# Chebyschev polynomials

size = 256 + 1
number = 12
cheby = []

lin = np.linspace(-1, 1, size)
chm = [1] * size                # F_{n-1}
chn = lin  # F_{n}

for i in range(number):
    y = 2 * lin * chn - chm
    chm = chn
    chn = y
    cheby.append(chm)
    # cheby.append([s1_15(i) for i in chm])

data['cheby'] = cheby

# Wavefolder
# https://www.desmos.com/calculator/7usex8budt

size = 1024 + 1
folds = 6
x = np.linspace(0, folds, size)
g = 1. / (1 + abs(x))
fold = g * (x + np.sin(16 * x * g))

data['fold'] = fold

# plt.plot(fold)
# plt.show()

# Generate

compile("data", "Data", data);
