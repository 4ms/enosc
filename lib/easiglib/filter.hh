#include "numtypes.hh"
#include "util.hh"
#include "buffer.hh"
#include "math.hh"

#pragma once

struct OnePoleLp {
  void Process(Float coef, Float input, Float *output) {
    state_ += (input - state_) * coef;
    *output = state_;
  }
private:
  Float state_;
};

struct OnePoleHp : OnePoleLp {
  Float Process(Float coef, Float input, Float *output) {
    OnePoleLp::Process(coef, input, output);
    return input - *output;
  }
};

template<class T, int SHIFT>
struct IOnePoleLp {
  void Process(T input, T *output) {
    state_ += input.template div2<SHIFT>() - state_.template div2<SHIFT>();
    *output = state_;
  }
private:
  T state_ = T(0_f);
};

// TODO: convert to numtypes
// complexity:
// size=size * (stages+1) * 4 + 4,
// time=O(stages)
template<int SIZE, int STAGES>
class Average {
  static constexpr int gain = ipow(SIZE - 1, STAGES);
  static_assert(gain < (1<<16), "Error: gain must be less than 65536");
  uint32_t state_i[STAGES] = {0};
  RingBuffer<uint32_t, SIZE> state_c[STAGES];
public:
  uint16_t Process(uint16_t x) {
    uint32_t y = x;
    for (int i=0; i<STAGES; i++) {
      state_c[i].Write(y);
      y -= state_c[i].ReadLast();
    }

    state_i[0] += y;
    for (int i=1; i<STAGES; i++) {
      state_i[i] += state_i[i-1];
    }
    uint32_t z = state_i[STAGES-1];
    return z / gain;
  }
};

struct FourPoleLadderLp {
  OnePoleLp lp_[4];
  Float fb = 0_f;
  void Process(Float in, Float *out, Float cutoff, Float resonance) {
    in -= fb * resonance;
    for (int i=0; i<4; i++)
      lp_[i].Process(cutoff, in, &in);
    *out = fb = Math::fast_tanh(in);
  }
};

enum class FilterMode {
  LOW_PASS,
  BAND_PASS,
  BAND_PASS_NORMALIZED,
  HIGH_PASS
};

enum class FreqApprox {
  EXACT,
  ACCURATE,
  FAST,
  DIRTY
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

  template<FreqApprox approximation>
  static Float tan(Float f) {
    // if (approximation == FREQUENCY_EXACT) {
    //   // Clip coefficient to about 100.
    //   f = f < 0.497f ? f : 0.497f;
    //   return tanf(Floats::pi * f);
    // } else 
    if (approximation == FreqApprox::DIRTY) {
      // Optimized for frequencies below 8kHz.
      const Float a = 3.736e-01_f * pi_pow_3;
      return f * (Math::pi + a * f * f);
    } else if (approximation == FreqApprox::FAST) {
      // The usual tangent approximation uses 3.1755e-01 and 2.033e-01, but
      // the coefficients used here are optimized to minimize error for the
      // 16Hz to 16kHz range, with a sample rate of 48kHz.
      const Float a = 3.260e-01_f * pi_pow_3;
      const Float b = 1.823e-01_f * pi_pow_5;
      Float f2 = f * f;
      return f * (Math::pi + f2 * (a + b * f2));
    } else if (approximation == FreqApprox::ACCURATE) {
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
  Svf() { set<FreqApprox::DIRTY>(0.01_f, 100.0_f); }

  // Set frequency and resonance from true units. Various approximations
  // are available to avoid the cost of tanf.
  template<FreqApprox approximation>
  void set(Float f, Float resonance) {
    g_ = Svf::tan<approximation>(f);
    r_ = 1.0_f / resonance;
    h_ = 1.0_f / (1.0_f + r_ * g_ + g_ * g_);
  }

  void set(Float f, Float resonance) {
    g_ = Svf::tan<FreqApprox::DIRTY>(f);
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
    
    if (mode == FilterMode::LOW_PASS) {
      return lp;
    } else if (mode == FilterMode::BAND_PASS) {
      return bp;
    } else if (mode == FilterMode::BAND_PASS_NORMALIZED) {
      return bp * r_;
    } else if (mode == FilterMode::HIGH_PASS) {
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
      *out++ = Process<FilterMode::BAND_PASS>(*in++);
    }
  }
  void ProcessBPAdd(Float amplitude, const Float* in, Float* out, size_t size) {
    while (size--) {
      *out++ += amplitude * Process<FilterMode::BAND_PASS>(*in++);
    }
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