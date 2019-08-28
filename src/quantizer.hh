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
  Grid() {}
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

  void copy_from(const Grid& g) {
    size_ = g.size_;
    std::copy(std::begin(g.grid_), std::end(g.grid_), grid_);
  }
};

class PreGrid : Nocopy {
  f grid_[kMaxGridSize];
  int size_ = 0;
public:
  bool add(f x) {
    if (size_ < kMaxGridSize-2) { // -2: if Octave wrap, will add one
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
  bool copy_to(Grid *g, bool wrap_octave) {
    // sort table
    std::sort(grid_, grid_+size_);
    // normalize from smallest element
    f base = grid_[0];
    for (f& x : grid_) {
      x -= base;
    }

    if (wrap_octave) {
      f max = grid_[size_-1];
      grid_[size_++] = ((max / 12_f).integral() + 1_f) * 12_f;
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
  Grid grids_[kBankNr][kGridNr];

  Grid const default_grids_[kBankNr][kGridNr] = {{
      // 12TET
      {0_f, 12_f},              // octave
      {0_f, 7_f, 12_f},         // octave+fifth
      {0_f, 4_f, 7_f, 12_f},    // major triad
      {0_f, 3_f, 7_f, 12_f},    // minor triad
      {0_f, 3_f, 5_f, 7_f, 12_f},    // minor triad
      // {0_f, 3_f, 5_f, 10_f, 12_f}, // ??
      {0_f, 7_f},               // circle of fifths
      {0_f, 5_f, 7_f},
      {0_f, 3_f, 5_f, 7_f},
      {0_f, 5_f},
      {0_f, 1_f},                // all semitones
    }, {
      // OCTAVE

      // Harmonics

      // harmonics 2-3 (perfect fifth)
      { 0_f, 7.0195500086_f, 12_f, },

      // harmonics 4-7
      {0_f, 3.86313714_f, 7.01955001_f, 9.68825906_f, 12_f },

      // harmonics 8-15
      {0_f, 2.03910002_f, 3.86313714_f, 5.51317942_f, 7.01955001_f,
       8.40527662_f, 9.68825906_f, 10.88268715_f, 12_f },

      // odd harmonics 1-16 (8)
      { 0_f, 7.0195500086_f, 15.86313713_f, 21.68825906_f, 26.03910001_f,
        29.51317942_f, 32.40527661_f, 34.88268714_f, 36_f },

      // Pythagorean scales

      // Do Fa So. np.log2([1., 4./3, 3./2, 2.]) * 12.
      { 0_f, 4.98044999_f, 7.01955001_f, 12_f },

      // Do Re Fa So La. np.log2([1., 9./8, 4./3, 3./2, 27./16, 2.]) * 12.
      { 0_f, 2.03910002_f, 4.98044999_f, 7.01955001_f,
        9.05865003_f,  12_f },

      // // np.log2([1., 9./8, 81./64, 4./3, 3./2, 27./16, 243./128, 2.]) * 12.
      // { 0_f, 2.03910002_f, 4.07820003_f, 4.98044999_f,
      //   7.01955001_f, 9.05865003_f, 11.09775004_f,  12_f },

      // Just scales

      // np.log2(sorted([1., 3./2, 4./3, 5./3, 2.])) * 12.
      { 0_f, 4.98044999_f, 7.01955001_f, 8.84358713_f, 12_f },

      // np.log2(sorted([1., 3./2, 4./3, 5./3, 5./4, 7./4, 2.])) * 12.
      { 0_f, 3.86313714_f, 4.98044999_f, 7.01955001_f, 8.84358713_f, 9.68825906_f, 12_f },

      // N-TET: np.log2(np.exp2(np.arange(1.+N)/N)) * 12.

      // 5-TET
      { 0_f ,  2.4_f,  4.8_f,  7.2_f,  9.6_f , 12_f},

      // 7-TET
      { 0_f, 1.71428571_f, 3.42857143_f, 5.14285714_f,
        6.85714286_f, 8.57142857_f, 10.28571429_f, 12_f },

    }, {
      // FREE
      {0_f, 7.0195500086_f},     // perfect fifths spiral
      {0_f, 2_f},                // whole tone = MLT#1
      {0_f, 1_f, 3_f},           // MLT #2
      {0_f, 2_f, 3_f, 4_f},      // MLT #3
      {0_f, 1_f, 2_f, 5_f, 6_f}, // MLT #4

      // Bohlen-Pierce
      { 0_f, 1.46304231_f, 2.92608462_f, 4.38912693_f,
        5.85216923_f, 7.31521154_f, 8.77825385_f, 10.24129616_f,
        11.70433847_f, 13.16738078_f, 14.63042308_f, 16.09346539_f,
        17.5565077_f, 19.01955001_f },

      // Bohlen-Pierce major triad
      { 0_f, 8.77825385_f, 14.63042308_f, 19.01955001_f },

      // Bohlen-Pierce minor triad
      { 0_f, 5.85216923_f, 14.63042308_f, 19.01955001_f },

      // Carlos alpha
      { 0_f, 0.78_f },

      // Carlos beta
      { 0_f, 0.638_f },
    }};

public:
  Quantizer() {
    // copy default grids to actual grids
    for (int i=0; i<kBankNr; ++i) {
      for (int j=0; j<kGridNr; ++j) {
        grids_[i][j].copy_from(default_grids_[i][j]);
      }
    }
  }

  void reset_grid(Parameters::Grid grid) {
    int i=grid.mode;
    int j=grid.value;
    grids_[i][j].copy_from(default_grids_[i][j]);
  }

  Grid *get_grid(Parameters::Grid grid) {
    return &grids_[grid.mode][grid.value];
  }
};
