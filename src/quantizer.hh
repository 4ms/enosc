#pragma once

#include "buffer.hh"

constexpr const int kGridNr = 10;
constexpr const int kMaxGridSize = 32;
constexpr const f kGridUnicityThreshold = 0.1_f;

struct PitchPair {
  f p1, p2, crossfade;
};

class Grid {
  f grid_[kMaxGridSize];
  int size_ = 0;
  friend class PreGrid;
public:
  Grid() {
    grid_[0] = 0_f;
    grid_[1] = 3_f;
    grid_[2] = 7_f;
    grid_[3] = 12_f;
    size_ = 4;
  }

  PitchPair Process(f const pitch) const {
    f max = grid_[size_-1];
    // quotient by the max
    int oct = (pitch / max).floor();
    f octaves = f(oct) * max;
    f semitones = pitch - octaves;
    // invariant: 0 < semitones < max
    int index = binary_search(semitones, grid_, size_);
    f p1 = grid_[index];
    f p2 = grid_[index+1];
    f crossfade = (semitones - p1) / (p2 - p1);
    p1 += octaves;
    p2 += octaves;

    if ((index + (oct * (size_+1))) & 1) {
      crossfade = 1_f - crossfade;
      f tmp = p1;
      p1 = p2;
      p2 = tmp;
    }

    return {p1, p2, crossfade};
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
  void copy_to(Grid *g) {
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
    std::copy(grid_, grid_+size_, g->grid_);
    g->size_=size_;
  }
};

class Quantizer {
  Grid grids_[kGridNr];
public:
  Grid *get_grid(Parameters::Grid grid) {
    return &grids_[grid.value];
  }
  // TODO temp
  Grid *get_grid(int i) {
    return &grids_[i];
  }
};
