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

size = 1024
spc = np.arange(0., size) / size
exp2 = 2 ** spc

data['exp2_u0_23'] = (exp2*(2**23)).astype(np.uint32)

# normalization factors for sums of uncorrelated signals

# we compute the pdfs of the repeated sums of uniform distributions
# [-1..1], and for each take the value at which they pass a certain threshold
# of "acceptable distortion probability"

size = 17
resolution = 512
threshold = 0.0001

u = np.ones(resolution)
v = u
factors = []

for i in range(size):
    # plt.plot(u)
    half = u[int(len(u)/2):]
    half = np.append(half, 0.)
    factor = np.argmax(half<threshold) / float(resolution/2)
    factors.append(1.0 / factor)
    u = np.convolve(u, v) / resolution

factors = factors * np.arange(1., size+1) - 1

# exception: attenuate normalization factor for 1 oscillator. This
# avoids clipping the output with the current (imperfect)
# normalization
factors[1] = -0.4

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

# Trianges waveshaper

triangles = [
    [0., 0.25, 0.5, 0.75, 1.],
    [0., 0.33333, 0.66666, 1., 1.],
    [0., 0.5, 1., 1., 1.],
    [0., 1., 1., 1., 1.],
    [0., 0.5, 1.0, 0.5, 1.],
    [0., 1., 0., 0.5, 1.],
    [0., 1.0, -1.0, 0.5, 1.],
    [0., 1.0, -1.0, 1.0, -1.0],
]

triangles = [[-x for x in i][::-1][:-1] + i for i in triangles]

data['triangles'] = triangles

# Harmonic series in pitch

size = 16
harm = np.arange(1,size+1)
harm = 12 * np.log2(harm) - 12

data['harm'] = harm

# Generate

compile("data", "Data", data);
