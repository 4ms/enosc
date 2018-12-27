#pragma once

#include "buffer.hh"

constexpr const int kGridNr = 10;
constexpr const int kMaxGridSize = 16;

class Grid {
  f grid[kMaxGridSize];
  int size = 0;

public:
  Grid() {
    grid[0] = 0_f;
    grid[1] = 3_f;
    grid[2] = 7_f;
    grid[3] = 12_f;
    size = 4;
  }

  void Process(f pitch, f &p1, f &p2, f &phase) {
    f max = grid[size-1];
    // quotient by the max
    int oct = (pitch / max).floor();
    f octaves = f(oct) * max;
    f semitones = pitch - octaves;
    // invariant: 0 < semitones < max
    int index = binary_search(semitones, grid, size);
    p1 = grid[index];
    p2 = grid[index+1];
    phase = (semitones - p1) / (p2 - p1);
    p1 += octaves;
    p2 += octaves;

    if ((index + (oct * (size+1))) & 1) {
      phase = 1_f - phase;
      f tmp = p1;
      p1 = p2;
      p2 = tmp;
    }
  }
};

class Quantizer {
  Grid grids_[kGridNr];
public:
  Grid get_grid(Parameters::Grid grid) {
    return grids_[grid.value];
  }
};
