#pragma once

#include "buffer.hh"

constexpr const int kGridNr = 10;
constexpr const int kBankNr = 3;
constexpr const int kMaxGridSize = 2 * kMaxNumOsc;
constexpr const f kGridUnicityThreshold = 0.1_f;

struct PitchPair {
  f p1, p2, crossfade;
};

class Grid : Nocopy {
  // grid_[0] = 0, contains [size_] sorted elements
  f grid_[kMaxGridSize];
  int size_ = 0;
  friend class PreGrid;
public:
  Grid(std::initializer_list<f> grid) {
    size_ = grid.size();
    std::copy(grid.begin(), grid.end(), grid_);
  }

  // pitch>0
  PitchPair Process(f const pitch) const {
    f max = grid_[size_-1];
    // quotient by the max
    f oct = (pitch / max).integral();
    f octaves = oct * max;
    f semitones = pitch - octaves;
    // semitones [0..max[
    int index = binary_search(semitones, grid_, size_);
    // index: [0..size_-2]
    f p1 = grid_[index];
    f p2 = grid_[index+1];
    f crossfade = (semitones - p1) / (p2 - p1);
    p1 += octaves;
    p2 += octaves;

    if ((index + (oct.floor() * (size_+1))) & 1) {
      crossfade = 1_f - crossfade;
      f tmp = p1;
      p1 = p2;
      p2 = tmp;
    }

    return {p1, p2, crossfade};
  }
};

class PreGrid : Nocopy {
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

  int size() const { return size_; }
  void clear() { size_ = 0; }
  f get(int i) const { return grid_[i]; }
  void set_last(f const x) { grid_[size_-1] = x; }
  bool remove_last() {
    if (size_ > 1) {
      size_--;
      return true;
    } else return false;
  }

  // do not call if size==0
  bool copy_to(Grid *g) {
    // sort table
    std::sort(grid_, grid_+size_);
    // normalize from smallest element
    f base = grid_[0];
    for (f& x : grid_) {
      x -= base;
    }
    // remove duplicate elements
    uniquify(grid_, size_, kGridUnicityThreshold);

    if (size_ > 1) {
      // copy to real grid
      std::copy(grid_, grid_+size_, g->grid_);
      g->size_=size_;
      return true;
    } else {
      return false;
    }
  }
};

class Quantizer {
  Grid grids_[kBankNr][kGridNr] = {{
      {0_f, 12_f},
      {0_f, 7_f, 12_f},
      {0_f, 5_f, 12_f},
      {0_f, 3_f, 8_f, 12_f},
      {0_f, 4_f, 7_f, 9_f, 12_f},
      {0_f, 3_f, 5_f, 10_f, 12_f},
      {0_f, 1_f, 5_f, 8_f, 12_f},
      {0_f, 2_f, 4_f, 8_f, 9_f, 12_f},
      {0_f, 2_f, 3_f, 5_f, 7_f, 9_f, 11_f, 12_f},
      {0_f, 1_f, 2_f, 3_f, 4_f, 5_f, 6_f, 7_f, 8_f, 9_f, 10_f, 11_f, 12_f},
    }, {
      {0_f, 1_f, 2_f, 3_f, 4_f, 5_f, 6_f, 7_f, 8_f, 9_f, 10_f, 11_f, 12_f},
      {0_f, 2_f, 3_f, 5_f, 7_f, 9_f, 11_f, 12_f},
      {0_f, 2_f, 4_f, 8_f, 9_f, 12_f},
      {0_f, 1_f, 5_f, 8_f, 12_f},
    }, {
      {0_f, 1_f, 2_f, 3_f, 4_f, 5_f, 6_f, 7_f, 8_f, 9_f, 10_f, 11_f, 12_f},
      {0_f, 2_f, 3_f, 5_f, 7_f, 9_f, 11_f, 12_f},
      {0_f, 2_f, 4_f, 8_f, 9_f, 12_f},
      {0_f, 1_f, 5_f, 8_f, 12_f},
    }};
public:
  Grid *get_grid(Parameters::Grid grid) {
    return &grids_[grid.mode][grid.value];
  }
};
