#include "numtypes.hh"
#include "filter.hh"
#include "units.hh"

#pragma once

struct Random {
  static uint32_t state() { return state_; }
  static uint32_t Word() {
    state_ = state_ * 1664525L + 1013904223L;
    return state();
  }
  static int16_t Int16() { return Word() >> 16; }
  // float between 0 and 1
  static Float Float01() { return Float(Word()) / Float(UINT32_MAX); }
  static constexpr uint32_t rand_max = UINT32_MAX;
private:
  static uint32_t state_;
};

uint32_t Random::state_ = 0x21;

// N number of stages, R decimation rate
// careful: gain must be <= 2^16
template<int N, int R>
class CicDecimator {
  int32_t hi[N] = {0};
  int32_t hc[N] = {0};
  static constexpr int gain = ipow(R, N);
public:
  // reads [R*size] input samples, writes [size] output samples:
  void Process(int16_t *input, int16_t *output, int size) {
    while(size--) {
      // N integrators
      for (int i=0; i<R; i++) {
        hi[0] += *input++;
        for(int n=1; n<N; n++) {
          hi[n] += hi[n-1];
        }
      }
      // N combs
      int32_t v = hi[N-1];
      for (int n=0; n<N; n++) {
        int32_t in = v;
        v -= hc[n];
        hc[n] = in;
      }
      *output++ = static_cast<int16_t>(v / gain);
    }
  }
};

// N number of stages, R interpolation rate
// careful: gain must be <= 2^16
template<int N, int R>
class CicInterpolator {
  int32_t hi[N] = {0};
  int32_t hc[N] = {0};
  static constexpr int gain = ipow(R, N-1);
public:
  // reads [size] input samples, writes [R*size] output samples:
  void Process(int16_t *input, int16_t *output, int size) {
    while(size--) {
      // N combs
      int32_t v = *input++;
      for (int n=0; n<N; n++) {
        int32_t in = v;
        v -= hc[n];
        hc[n] = in;
      }
      // N integrators
      for (int i=0; i<R; i++) {
        hi[0] += i==0 ? v : 0;
        for(int n=1; n<N; n++) {
          hi[n] += hi[n-1];
        }
        *output++ = static_cast<int16_t>(hi[N-1] / gain);
      }
    }
  }
};

template<int N, int R>
class PdmFilter : CicDecimator<N, R> {

  const int16_t setbits[256] = {
#   define S(n) (int16_t)((2*(n)-8) << 11)
#   define B2(n) S(n),  S(n+1),  S(n+1),  S(n+2)
#   define B4(n) B2(n), B2(n+1), B2(n+1), B2(n+2)
#   define B6(n) B4(n), B4(n+1), B4(n+1), B4(n+2)
    B6(0), B6(1), B6(1), B6(2)
  };
public:
  // reads [8*R*size] binary input sample, outputs [size] samples
  void Process(uint8_t *input, int16_t *output, int size) {
    int16_t temp[R * size];
    for (int i=0; i<size * R; i++) {
      temp[i] = setbits[*input];
      input++;
    }
    CicDecimator<N, R>::Process(temp, output, size);
  }
};


template<int S> struct PinkNoiseCsts {};
template<> struct PinkNoiseCsts<3> {
  static constexpr Float A[3] = {0.02109238_f, 0.07113478_f, 0.68873558_f};
  static constexpr Float P[3] = {0.3190_f, 0.7756_f, 0.9613_f};
};
constexpr Float PinkNoiseCsts<3>::A[];
constexpr Float PinkNoiseCsts<3>::P[];

template<> struct PinkNoiseCsts<4> {
  static constexpr Float A[4] = {0.0186944045911_f, 0.0514485907859_f, 0.19168_f, 0.940098715596_f};
  static constexpr Float P[4] = {0.3030_f, 0.7417_f, 0.9168_f, 0.9782_f};
};
constexpr Float PinkNoiseCsts<4>::A[];
constexpr Float PinkNoiseCsts<4>::P[];

template<> struct PinkNoiseCsts<5> {
  static constexpr Float A[5] = {0.0030662336401_f, 0.0153123333238_f, 0.0536773305003_f, 0.191430730313_f, 0.999749211196_f};
  static constexpr Float P[5] = {0.15571_f, 0.30194_f, 0.74115_f, 0.93003_f, 0.98035_f};
};
constexpr Float PinkNoiseCsts<5>::A[];
constexpr Float PinkNoiseCsts<5>::P[];

template<int STAGES>
class PinkNoise {
  Float state[STAGES] = {0};

  static constexpr Float offset(int i) {
    return i<0 ? 0 : PinkNoiseCsts<STAGES>::A[i] + offset(i-1);
  }
  static constexpr Float offset() { return offset(STAGES); }

public:
  void Process(Float *out) {
    static const Float RMI2 = 2.0 / Random::rand_max;

    Float s = 0.0f;

    for(int i=0; i<STAGES; i++) {
      Float temp = Random::Word();
      state[i] = PinkNoiseCsts<STAGES>::P[i] * (state[i] - temp) + temp;
      s += PinkNoiseCsts<STAGES>::A[i] * state[i];
    }

    *out = s * RMI2 - offset();
  }
};


// Magic circle algorithm
class MagicSine {
  Float sinz = 0_f;
  Float cosz = 1_f;
  Float f = 2_f * Math::pi * 0.001_f;
public:
  MagicSine(Float freq) : f(2_f * Math::pi * freq) { }

  void Process(Float *out) {
    sinz += f * cosz;
    cosz -= f * sinz;
    *out = sinz;
  }

  void set_frequency(Float freq) {  // freq = f_n / f_s
    f = 2.0_f * Math::pi * freq;
  // this is an approximation, ok for small frequencies. The actual
  // value is f = 2sin(pi f_n T) (T sampling period, f_n freq)
  }
};
