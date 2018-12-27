#include "buffer.hh"

constexpr const int kGridNr = 10;
constexpr const f kRepeatSt = 14_f;

class Grid {
  f grid[5] = {0_f, 3_f, 7_f, 10_f, f(kRepeatSt)};
  int size = 5;

public:
  void Process(f pitch, f &p1, f &p2, f &phase) {

    int oct = (pitch / kRepeatSt).floor();
    f octaves = f(oct) * kRepeatSt;
    f semitones = pitch - octaves;

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
