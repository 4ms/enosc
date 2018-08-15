#include "numtypes.hh"
#include "data.hh"

#pragma once

struct Math {
  static constexpr f pi = f(3.14159265358979323846264338327950288);


  // [-1; +1] --> [-1; +1]
  static constexpr f faster_sine(f x) {
    x = (x * 2_f) - 1_f;
    return 4_f * (x - x * x.abs());
  }

  // [-1; +1] --> [-1; +1]
  static constexpr s1_15 faster_sine(u0_32 x) {
    s1_31 y = x.to_wrap<SIGNED, 1, 31>() - 1._s1_31;
    s1_15 z = (y * 2).to_narrow<1,15>();
    z -= z * z.abs();
    z *= 2;
    z = z.add_sat(z);
    return z;
  }

  // [-1; +1] --> [-1; +1]
  static constexpr f fast_sine(f x) {
    f y = faster_sine(x);
    y = 0.225_f * (y * y.abs() - y) + y;
    return y;
  }

};

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

struct Freq {
  static Float Hz(Float x) { return x / Float(kSampleRate); }

  static Float semitones_to_ratio(Float semitones) { // -127..128
    semitones += 128_f;
    int integral = s(semitones).repr(); // 0..255
    Float fractional = semitones - Float(integral);    // 0..1
    int low_index = s(fractional * Float(Data::pitch_ratios_low.size())).repr();
    return
      // TODO: better solution for data lookup ~> Float
      Float(Data::pitch_ratios_high[integral]) *
      Float(Data::pitch_ratios_low[low_index]);
  }

  static Float of_midi(Float midi_pitch) {
    return semitones_to_ratio(midi_pitch - 69_f) * 440_f;
  }
};

// TODO: saturating instruction
short clip16(int32_t x) {
  return
    x > INT16_MAX ? INT16_MAX :
    x < INT16_MIN ? INT16_MIN :
    x;
}

Float soft_clip(Float x) {
  return x * (27.0_f + x * x) / (27.0_f + 9.0_f * x * x);
}

short short_of_float(Float x) {
  return clip16(s(x * Float(INT16_MAX)).repr());
}

Float float_of_short(short x) {
  return Float(x) / Float(INT16_MAX);
}

constexpr int ipow(int a, int b) {
  return b==0 ? 1 : a * ipow(a, b-1);
}

template<typename T, int SIZE>
class RingBuffer {
  T buffer_[SIZE] = {0};
  uint32_t cursor_ = SIZE;
public:
  void Write(T x) {
    buffer_[++cursor_ % SIZE] = x;
  }
  T Read(uint32_t n) {
    return buffer_[(cursor_ - n) % SIZE];
  }
  T ReadLast() {
    return buffer_[(cursor_+1) % SIZE];
  }
};


template<int SIZE>
struct RingBuffer<Float, SIZE> {
  void Write(Float x) {
    buffer_[++cursor_ % SIZE] = x;
  }
  Float Read(int n) {
    return buffer_[(cursor_ - n) % SIZE];
  }
  Float ReadLast() {
    return buffer_[(cursor_+1) % SIZE];
  }

  Float ReadLinear(Float x) {
    uint32_t index = static_cast<uint32_t>(x);
    Float fractional = x - Float(index);
    Float x1 = buffer_[(cursor_ - index+1) % SIZE];
    Float x2 = buffer_[(cursor_ - index) % SIZE];
    return x1 + (x2 - x1) * fractional;
  }
private:
  Float buffer_[SIZE] = {0};
  uint32_t cursor_ = SIZE;
};

template<int coef_num, int coef_denom>
struct OnePoleLp {
  void Process(Float input, Float *output) {
    state_ += (input - state_) * coef;
    *output = state_;
  }
private:
  Float state_;
  static constexpr Float coef = Float(coef_num) / Float(coef_denom);
};

template<int coef_num, int coef_denom>
class OnePoleHp : public OnePoleLp<coef_num, coef_denom> {
public:
  Float Process(Float input) {
    return input - OnePoleLp<coef_num, coef_denom>::Process(input);
  }
};

// careful: gain must be <= 2^16
// complexity:
// size=size * (stages+1) * 4 + 4,
// time=O(stages)
template<int SIZE, int STAGES>
class Average {
  static constexpr int gain = ipow(SIZE - 1, STAGES);
  int32_t state_i[STAGES] = {0};
  RingBuffer<int32_t, SIZE> state_c[STAGES];
public:
  int16_t Process(int16_t x) {
    int32_t y = x;
    for (int i=0; i<STAGES; i++) {
      state_c[i].Write(y);
      y -= state_c[i].ReadLast();
    }

    state_i[0] += y;
    for (int i=1; i<STAGES; i++) {
      state_i[i] += state_i[i-1];
    }
    int32_t z = state_i[STAGES-1];
    return z / gain;
  }
};

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

template <typename T>
class SlewLimiter {
  T state_ = {0};
  T slew_up_, slew_down_;
public:
  SlewLimiter(T slew_up, T slew_down) :
    slew_up_(slew_up), slew_down_(-slew_down) {}
  void Process(T input, T *output) {
    T error = input - state_;
    if (error > slew_up_) error = slew_up_;
    if (error < slew_down_) error = slew_down_;
    state_ += error;
    *output = state_;
  }

  void Process(T *input, T *output, int size) {
    while(size--) Process(*input++, output++);
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


enum FilterMode {
  FILTER_MODE_LOW_PASS,
  FILTER_MODE_BAND_PASS,
  FILTER_MODE_BAND_PASS_NORMALIZED,
  FILTER_MODE_HIGH_PASS
};

enum FrequencyApproximation {
  FREQUENCY_EXACT,
  FREQUENCY_ACCURATE,
  FREQUENCY_FAST,
  FREQUENCY_DIRTY
};

static Float const pi_pow_2 = Math::pi * Math::pi;
static Float const pi_pow_3 = pi_pow_2 * Math::pi;
static Float const pi_pow_5 = pi_pow_3 * pi_pow_2;
static Float const pi_pow_7 = pi_pow_5 * pi_pow_2;
static Float const pi_pow_9 = pi_pow_7 * pi_pow_2;
static Float const pi_pow_11 = pi_pow_9 * pi_pow_2;

class Svf {
  Float g_ = 0_f;
  Float r_ = 0_f;
  Float h_ = 0_f;

  Float state_1_ = 0_f;
  Float state_2_ = 0_f;

  template<FrequencyApproximation approximation>
  static Float tan(Float f) {
    // if (approximation == FREQUENCY_EXACT) {
    //   // Clip coefficient to about 100.
    //   f = f < 0.497f ? f : 0.497f;
    //   return tanf(Floats::pi * f);
    // } else 
    if (approximation == FREQUENCY_DIRTY) {
      // Optimized for frequencies below 8kHz.
      const Float a = 3.736e-01_f * pi_pow_3;
      return f * (Math::pi + a * f * f);
    } else if (approximation == FREQUENCY_FAST) {
      // The usual tangent approximation uses 3.1755e-01 and 2.033e-01, but
      // the coefficients used here are optimized to minimize error for the
      // 16Hz to 16kHz range, with a sample rate of 48kHz.
      const Float a = 3.260e-01_f * pi_pow_3;
      const Float b = 1.823e-01_f * pi_pow_5;
      Float f2 = f * f;
      return f * (Math::pi + f2 * (a + b * f2));
    } else if (approximation == FREQUENCY_ACCURATE) {
      // These coefficients don't need to be tweaked for the audio range.
      const Float a = 3.333314036e-01_f * pi_pow_3;
      const Float b = 1.333923995e-01_f * pi_pow_5;
      const Float c = 5.33740603e-02_f * pi_pow_7;
      const Float d = 2.900525e-03_f * pi_pow_9;
      const Float e = 9.5168091e-03_f * pi_pow_11;
      Float f2 = f * f;
      return f * (Math::pi + f2 * (a + f2 * (b + f2 * (c + f2 * (d + f2 * e)))));
    }
  }

 public:
  Svf() { set<FREQUENCY_DIRTY>(0.01_f, 100.0_f); }

  // Set frequency and resonance from true units. Various approximations
  // are available to avoid the cost of tanf.
  template<FrequencyApproximation approximation>
  void set(Float f, Float resonance) {
    g_ = Svf::tan<approximation>(f);
    r_ = 1.0_f / resonance;
    h_ = 1.0_f / (1.0_f + r_ * g_ + g_ * g_);
  }

  void set(Float f, Float resonance) {
    g_ = Svf::tan<FREQUENCY_DIRTY>(f);
    r_ = 1.0_f / resonance;
    h_ = 1.0_f / (1.0_f + r_ * g_ + g_ * g_);
  }
  
  template<FilterMode mode>
  Float Process(Float in) {
    Float hp, bp, lp;
    hp = (in - r_ * state_1_ - g_ * state_1_ - state_2_) * h_;
    bp = g_ * hp + state_1_;
    state_1_ = g_ * hp + bp;
    lp = g_ * bp + state_2_;
    state_2_ = g_ * bp + lp;
    
    if (mode == FILTER_MODE_LOW_PASS) {
      return lp;
    } else if (mode == FILTER_MODE_BAND_PASS) {
      return bp;
    } else if (mode == FILTER_MODE_BAND_PASS_NORMALIZED) {
      return bp * r_;
    } else if (mode == FILTER_MODE_HIGH_PASS) {
      return hp;
    }
  }
  
  template<FilterMode mode>
  void Process(const Float* in, Float* out, size_t size) {
    while (size--) {
      *out++ = Process<mode>(*in++);
    }
  }

  void ProcessBP(const Float* in, Float* out, size_t size) {
    while (size--) {
      *out++ = Process<FILTER_MODE_BAND_PASS>(*in++);
    }
  }
  void ProcessBPAdd(Float amplitude, const Float* in, Float* out, size_t size) {
    while (size--) {
      *out++ += amplitude * Process<FILTER_MODE_BAND_PASS>(*in++);
    }
  }
};

// Magic circle algorithm
class SineOscillator {
  Float sinz = 0_f;
  Float cosz = 1_f;
  Float f = 2_f * Math::pi * 0.001_f;
public:
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
