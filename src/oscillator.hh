#pragma once

#include "dsp.hh"
#include "distortion.hh"

constexpr const f kMaxModulationIndex = 4_f / f(kNumOsc);

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
  IFloat fader {0_f};

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
  void Process(u0_32 const freq, f const twist, f const warp,
               f const fade, f const amplitude, f const modulation,
               Block<u0_16> mod_in, Block<u0_16> mod_out, Block<f> sum_output) {
    fader.set(fade, sum_output.size());
    u0_16 *mod_in_it = mod_in.begin();
    u0_16 *mod_out_it = mod_out.begin();
    for (f &sum : sum_output) {
      u0_16 &m_in = *mod_in_it;
      u0_16 &m_out = *mod_out_it;
      u0_32 mod = u0_32(m_in);
      f sample = Process<twist_mode, warp_mode>(freq, mod, twist, warp);
      sample *= fader.next();
      m_out += u0_16((sample + 1_f) * modulation * kMaxModulationIndex);
      sum += sample * amplitude;
      mod_in_it++;
      mod_out_it++;
    }
  }
};

struct FrequencyPair { f freq1, freq2, crossfade; };

class OscillatorPair : Nocopy {
  Oscillator osc_[2];
  bool frozen;
  FrequencyPair previous_freq;

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
  void set_freeze(bool b) { frozen = b; }

  template<TwistMode twist_mode, WarpMode warp_mode>
  void Process(FrequencyPair freq,
               f const twist, f const warp, f const amplitude, f const modulation,
               Block<u0_16> mod_in, Block<u0_16> mod_out,
               Block<f> sum_output) {
    if (frozen) freq = previous_freq;
    else previous_freq = freq;

    f crossfade = freq.crossfade * freq.crossfade;             // helps find the 0 point
    f fade1 = 1_f - freq.crossfade;
    f fade2 = freq.crossfade;

    f aliasing_factor1 = freq.freq1; // TODO
    fade1 *= antialias(aliasing_factor1);
    f aliasing_factor2 = freq.freq2; // TODO
    fade2 *= antialias(aliasing_factor2);

    mod_out.fill(0._u0_16);
    osc_[0].Process<twist_mode, warp_mode>(u0_32(freq.freq1), twist, warp,
                                           fade1, amplitude, modulation,
                                           mod_in, mod_out, sum_output);
    osc_[1].Process<twist_mode, warp_mode>(u0_32(freq.freq2), twist, warp,
                                           fade2, amplitude, modulation,
                                           mod_in, mod_out, sum_output);
  }
};
