#pragma once

#include "dsp.hh"

class Phasor : Nocopy {
  u0_32 phase_ = u0_32::of_repr(Random::Word());
public:
  u0_32 Process(u0_32 freq) {
    phase_ += freq;
    return phase_;
  }
};

class SineShaper : Nocopy {
  s1_15 history_ = 0._s1_15;
  IOnePoleLp<s1_15, 2> lp_;
public:

  s1_15 Process(u0_32 phase, u0_16 feedback) {
    s1_31 fb = history_ * feedback.to_signed();
    phase += fb.to_unsigned() + u0_32(feedback);
    s1_15 sample = Data::short_sine.interpolate(phase).movl<15>();
    lp_.Process(sample, &history_);
    return sample;
  }

  // without Feedback
  s1_15 Process(u0_32 phase) {
    return Data::short_sine.interpolate(phase).movl<15>();
  }
};

class Oscillator : Phasor, SineShaper {
  // TODO: switching to IFloat -> -2% perf!!!
  IIFloat amplitude {0_f};

  // TODO optimize
  static f crush(s1_15 sample, f amount) {
    f x = sample.to_float();
    union { float a; uint32_t b; } t = {x.repr()};
    t.b ^= (uint32_t)((t.b & ((1 << 23)-1)) * amount.repr());
    // t.b ^= (int32_t)(((1<<23)-1) * amount.repr());
    return f(t.a);
  }

  // TODO optimize
  // TODO amount: [0..1[
  static f cheby(s1_15 x, f amount) {
    // TODO comprendre -2
    amount *= (Data::cheby.size() - 2_u32).to_float();
    index idx = index(amount);
    f frac = amount.fractional();
    u0_16 phase = x.to_unsigned_scale();
    f s1 = Data::cheby[idx].interpolate(phase);
    f s2 = Data::cheby[idx+1_u32].interpolate(phase);
    return Math::crossfade(s1, s2, frac);
  }

  static f fold(s1_15 x, f amount) {
    constexpr Buffer<f, 12> fold = {{-1_f, 1_f, -1_f, 1_f, -1_f, 1_f,
                                    -1_f, 1_f, -1_f, 1_f, -1_f, 1_f, -1_f}};
    f sample = x.to_float() * 0.5_f + 0.5_f;
    amount = (amount + 1_f/fold.size().to_float());
    f phase = sample * amount;
    return fold.interpolate(phase);
  }

  // TODO rewrite with Numtypes
  static u0_32 pulsar(u0_32 phase, f amount) {
    u0_16 p = u0_16::inclusive(amount);
    uint32_t x = (phase.repr() / (p.repr()+1));
    if (x > UINT16_MAX) x = UINT16_MAX;
    x <<= 16;
    return u0_32::of_repr(x);
  }

  // TODO rewrite and optimize
  static u0_32 decimate(u0_32 phase, f amount) {
    uint32_t x = phase.repr();
    x = (x ^ (uint32_t)(x*amount.repr())) * Math::sgn(x);
    x ^= (uint32_t)(UINT32_MAX/4 * amount.repr());
    return u0_32::of_repr(x);
  }

public:
  template<TwistMode twist_mode, WarpMode warp_mode>
  f Process(u0_32 freq, f twist, f warp) {

    u0_32 phase = Phasor::Process(freq);

    f feedback = 0_f;
    if (twist_mode == FEEDBACK) {
      feedback = twist;
    } else if (twist_mode == PULSAR) {
      phase = pulsar(phase, twist);
    } else if (twist_mode == DECIMATE) {
      phase = decimate(phase, twist);
    }

    s1_15 sine = twist_mode == FEEDBACK ?
      SineShaper::Process(phase, u0_16(feedback)) :
      SineShaper::Process(phase);

    f output;

    if (warp_mode == CRUSH) {
      output = crush(sine, warp);
    } else if (warp_mode == CHEBY) {
      output = cheby(sine, warp);
    } else if (warp_mode == FOLD) {
      output = fold(sine, warp);
    }
    return output;
  }

  template<TwistMode twist_mode, WarpMode warp_mode>
  void Process(u0_32 freq, f twist, f warp, f amp, Block<f> output) {
    amplitude.set(amp, output.size());
    for (f &out : output) {
      f sample = Process<twist_mode, warp_mode>(u0_32(freq), twist, warp);
      out += sample * amplitude.next();
    }
  }
};

class DoubleOscillator : Nocopy {
  Oscillator osc_[2];

  // simple linear piecewise function: 0->1, 0.5->1, 1->0
  static f antialias(f factor) {
    f amplitude = 1_f;
      if (factor > 0.5_f) {
        amplitude = 0_f;
      } else if (factor > 0.25_f) {
        amplitude *= 2_f - 4_f * factor;
      }
      return amplitude;
  }
public:
  template<TwistMode twist_mode, WarpMode warp_mode>
  void Process(f const freq1, f const freq2, f phase,
               f const twist, f const warp, f const amplitude,
               Block<f> output) {
    phase *= phase;             // helps find the 0 point
    f amp1 = amplitude * (1_f - phase);
    f amp2 = amplitude * phase;

    f aliasing_factor1 = freq1; // TODO
    amp1 *= antialias(aliasing_factor1);
    f aliasing_factor2 = freq2; // TODO
    amp2 *= antialias(aliasing_factor2);

    osc_[0].Process<twist_mode, warp_mode>(u0_32(freq1), twist, warp, amp1, output);
    osc_[1].Process<twist_mode, warp_mode>(u0_32(freq2), twist, warp, amp2, output);
  }
};
