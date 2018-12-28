#pragma once

#include "buffer.hh"

constexpr const int kGridNr = 10;
constexpr const int kMaxGridSize = 32;
constexpr const f kGridUnicityThreshold = 0.1_f;

class Grid {
  f grid[kMaxGridSize];
  int size = 0;
  friend class PreGrid;
public:
  Grid() {
    grid[0] = 0_f;
    grid[1] = 3_f;
    grid[2] = 7_f;
    grid[3] = 12_f;
    size = 4;
  }

  void Process(f const pitch, f &p1, f &p2, f &phase) const {
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

class PreGrid {
  f grid_[kMaxGridSize];
  int size_ = 0;
public:
  bool add(f x) {
    if (size_ < kMaxGridSize-1) {
      grid_[size_++] = x;
      return true;
    } else {
      return false;
    }
  }

  int size() { return size_; }
  void clear() { size_ = 0; }

  // do not call if size==0
  void copy_to(Grid &g) {
    // sort table
    std::sort(grid_, grid_+size_);
    // normalize from smallest element
    f base = grid_[0];
    for (f& x : grid_) {
      x -= base;
    }
    // remove duplicate elements
    uniquify(grid_, size_, kGridUnicityThreshold);

    // copy to real grid
    std::copy(grid_, grid_+size_, g.grid);
    g.size=size_;
  }
};

class Quantizer {
  Grid grids_[kGridNr];
public:
  Grid &get_grid(Parameters::Grid grid) {
    return grids_[grid.value];
  }
  // TODO temp
  Grid &get_grid(int i) {
    return grids_[i];
  }
};
