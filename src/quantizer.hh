
class Quantizer {
  f freqs[5] = {0_f, 3_f, 7_f, 10_f, 12_f};
  int size = 5;

public:
  void Process(f pitch, f &p1, f &p2, f &phase) {



    int32_t integral = (pitch / 6_f).floor();
    p1 = f(integral) * 6_f;
    p2 = p1 + 6_f;
    phase = (pitch - p1) / (p2 - p1);

    if (integral & 1) {
      phase = 1_f - phase;
      f tmp = p1;
      p1 = p2;
      p2 = tmp;
    }
  }
};
