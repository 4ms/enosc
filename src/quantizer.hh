#include "buffer.hh"

constexpr const f kRepeatSt = 14_f;

class Quantizer {
  f grid[5] = {0_f, 3_f, 7_f, 10_f, f(kRepeatSt)};
  int size = 5;

public:
  void Process(f pitch, f &p1, f &p2, f &phase) {

    int oct = (pitch / kRepeatSt).floor();
    f octaves = (pitch / kRepeatSt).integral() * kRepeatSt;
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
