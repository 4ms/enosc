#pragma once

#include "dsp.hh"

constexpr const f kMaxModulationIndex = 5_f / f(kNumOsc);

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

namespace Distortion {

  template<WarpMode> inline f warp(s1_15, f);
  template<TwistMode> inline u0_32 twist(u0_32, f);

  // TODO rewrite with Numtypes
  template<>
  inline u0_32 twist<PULSAR>(u0_32 phase, f amount) {
    u0_16 p = u0_16::inclusive(amount);
    uint32_t x = (phase.repr() / (p.repr()+1));
    if (x > UINT16_MAX) x = UINT16_MAX;
    x <<= 16;
    return u0_32::of_repr(x);
  }

  // TODO rewrite and optimize
  template<>
  inline u0_32 twist<DECIMATE>(u0_32 phase, f amount) {
    uint32_t x = phase.repr();
    x = (x ^ (uint32_t)(x*amount.repr())) * Math::sgn(x);
    x ^= (uint32_t)(UINT32_MAX/4 * amount.repr());
    return u0_32::of_repr(x);
  }

  template<>
  inline u0_32 twist<FEEDBACK>(u0_32 phase, f amount) {
    return phase;
  }

  // TODO optimize
  template<>
  inline f warp<CRUSH>(s1_15 sample, f amount) {
    f x = sample.to_float();
    union { float a; uint32_t b; } t = {x.repr()};
    t.b ^= (uint32_t)((t.b & ((1 << 23)-1)) * amount.repr());
    // t.b ^= (int32_t)(((1<<23)-1) * amount.repr());
    return f(t.a);
  }

  // TODO optimize
  // TODO amount: [0..1[
  template<>
  inline f warp<CHEBY>(s1_15 x, f amount) {
    // TODO comprendre -2
    amount *= (Data::cheby.size() - 2_u32).to_float();
    index idx = index(amount);
    f frac = amount.fractional();
    u0_16 phase = x.to_unsigned_scale();
    f s1 = Data::cheby[idx].interpolate(phase);
    f s2 = Data::cheby[idx+1_u32].interpolate(phase);
    return Math::crossfade(s1, s2, frac);
  }

  template<>
  inline f warp<FOLD>(s1_15 x, f amount) {
    constexpr Buffer<f, 12> fold = {{-1_f, 1_f, -1_f, 1_f, -1_f, 1_f,
                                    -1_f, 1_f, -1_f, 1_f, -1_f, 1_f, -1_f}};
    f sample = x.to_float() * 0.5_f + 0.5_f;
    amount = (amount + 1_f/fold.size().to_float());
    f phase = sample * amount;
    return fold.interpolate(phase);
  }
};

class Oscillator : Phasor, SineShaper {
  IFloat amplitude {0_f};

public:
  template<TwistMode twist_mode, WarpMode warp_mode>
  f Process(u0_32 freq, u0_32 mod, f twist_amount, f warp_amount) {
    u0_32 phase = Phasor::Process(freq);
    phase += mod;
    phase = Distortion::twist<twist_mode>(phase, twist_amount);

    s1_15 sine = twist_mode == FEEDBACK ?
      SineShaper::Process(phase, u0_16(twist_amount)) :
      SineShaper::Process(phase);

    return Distortion::warp<warp_mode>(sine, warp_amount);
  }

  template<TwistMode twist_mode, WarpMode warp_mode>
  void Process(u0_32 const freq, f const twist, f const warp, f const amp, f const modulation,
               Block<u0_16> mod_in, Block<u0_16> mod_out, Block<f> sum_output) {
    amplitude.set(amp, sum_output.size());
    u0_16 *mod_in_it = mod_in.begin();
    u0_16 *mod_out_it = mod_out.begin();
    for (f &sum : sum_output) {
      u0_16 &m_in = *mod_in_it;
      u0_16 &m_out = *mod_out_it;
      u0_32 mod = u0_32(m_in);
      f sample = Process<twist_mode, warp_mode>(freq, mod, twist, warp);
      sample *= amplitude.next();
      sum += sample;
      m_out += u0_16((sample + 1_f) * modulation * kMaxModulationIndex);
      mod_in_it++;
      mod_out_it++;
    }
  }
};


struct FrequencyPair { f freq1, freq2, crossfade; };

class OscillatorPair : Nocopy {
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
  void Process(FrequencyPair const p,
               f const twist, f const warp, f const amplitude, f const modulation,
               Block<u0_16> mod_in, Block<u0_16> mod_out,
               Block<f> sum_output) {
    f crossfade = p.crossfade * p.crossfade;             // helps find the 0 point
    f amp1 = amplitude * (1_f - p.crossfade);
    f amp2 = amplitude * p.crossfade;

    f aliasing_factor1 = p.freq1; // TODO
    amp1 *= antialias(aliasing_factor1);
    f aliasing_factor2 = p.freq2; // TODO
    amp2 *= antialias(aliasing_factor2);

    mod_out.fill(0._u0_16);
    osc_[0].Process<twist_mode, warp_mode>(u0_32(p.freq1), twist, warp, amp1, modulation,
                                           mod_in, mod_out, sum_output);
    osc_[1].Process<twist_mode, warp_mode>(u0_32(p.freq2), twist, warp, amp2, modulation,
                                           mod_in, mod_out, sum_output);
  }
};
