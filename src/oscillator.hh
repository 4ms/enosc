#pragma once

#include "dsp.hh"
#include "distortion.hh"
#include "dynamic_data.hh"

constexpr int kWaveformBufferSize = 256;

class Phasor {
  u0_32 phase_ = u0_32::of_repr(Random::Word());
public:
  Phasor() {}
  Phasor(u0_32 phase) : phase_(phase) {}
  u0_32 Process(u0_32 freq) {
    phase_ += freq;
    return phase_;
  }
};

class SineShaper {
  IOnePoleLp<s1_15, 2> lp_;
public:

  s1_15 Process(u0_32 phase, u0_16 feedback) {
    s1_31 fb = lp_.state() * feedback.to_signed();
    phase += fb.to_unsigned() + u0_32(feedback);
    s1_15 sample = DynamicData::sine.interpolateDiff<s1_15>(phase);
    lp_.Process(sample);
    return sample;
  }

  // without Feedback
  s1_15 Process(u0_32 phase) {
    return DynamicData::sine.interpolateDiff<s1_15>(phase);
  }
};

class Oscillator {
  Phasor phasor_;
  IFloat fader_ {0_f};

  static Buffer<f, kWaveformBufferSize+1> waveform;
  static SineShaper sh;

public:
  template<TwistMode twist_mode, WarpMode warp_mode>
  static void ComputeWaveform(f twist_amount, f warp_amount) {
    constexpr u0_32 freq = u0_32::max_val / kWaveformBufferSize;
    Phasor ph {0._u0_32};

    for (f& sample : waveform) {
      u0_32 phase = ph.Process(freq);
      phase = Distortion::twist<twist_mode>(phase, twist_amount);

      s1_15 sine = twist_mode == FEEDBACK ?
        sh.Process(phase, u0_16(twist_amount)) :
        sh.Process(phase);

      sample = Distortion::warp<warp_mode>(sine, warp_amount);
    }
  }

  template<int block_size>
  void Process(u0_32 const freq, f const fade, f const amplitude, f const modulation,
               Block<u0_16, block_size> mod_in, Block<u0_16, block_size> mod_out,
               Block<f, block_size> sum_output) {
    Phasor ph = phasor_;
    IFloat fd = fader_;
    fd.set(fade, block_size);

    for (auto [sum, m_in, m_out] : zip(sum_output, mod_in, mod_out)) {
      u0_32 phase = ph.Process(freq);
      phase += u0_32(m_in);
      f sample = waveform.interpolate(phase);
      sample *= fd.next();
      // TODO comprendre +1
      m_out += u0_16((sample + 1_f) * modulation);
      sum += sample * amplitude;
    }

    phasor_ = ph;
    fader_ = fd;
  }
};

inline Buffer<f, kWaveformBufferSize+1> Oscillator::waveform;
inline SineShaper Oscillator::sh;

struct FrequencyPair { f freq1, freq2, crossfade; };

class OscillatorPair : Nocopy {
  Oscillator osc_[2];
  bool frozen = false;
  FrequencyPair previous_freq;

  // simple linear piecewise function: 0->1, 0.25->1, 0.5->0
  static f antialias(f factor) {
    return (2_f - 4_f * factor).clip(0_f,1_f);
  }
public:
  void set_freeze(bool b) { frozen = b; }

  template<int block_size>
  void Process(FrequencyPair freq, f crossfade_factor,
               f const amplitude, f const modulation,
               Block<u0_16, block_size> mod_in, Block<u0_16, block_size> mod_out,
               Block<f, block_size> sum_output) {
    if (frozen) freq = previous_freq;
    else previous_freq = freq;

    f crossfade = freq.crossfade;

    // shape crossfade so notes are easier to find
    crossfade = Math::fast_raised_cosine(crossfade);
    crossfade = Signal::crop(crossfade_factor, crossfade);

    f fade1 = 1_f - crossfade;
    f fade2 = crossfade;

    f aliasing_factor1 = freq.freq1; // TODO
    fade1 *= antialias(aliasing_factor1);
    f aliasing_factor2 = freq.freq2; // TODO
    fade2 *= antialias(aliasing_factor2);

    // mod_out is accumulated in the two calls, so we need to zero it here
    mod_out.fill(0._u0_16);
    osc_[0].Process(u0_32(freq.freq1),
                    fade1, amplitude, modulation,
                    mod_in, mod_out, sum_output);
    osc_[1].Process(u0_32(freq.freq2),
                    fade2, amplitude, modulation,
                    mod_in, mod_out, sum_output);
  }
};
