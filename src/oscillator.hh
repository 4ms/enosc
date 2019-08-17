#pragma once

#include "dsp.hh"
#include "distortion.hh"
#include "dynamic_data.hh"

class Phasor {
  u0_32 phase_ = u0_32::of_repr(Random::Word());
public:
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
  SineShaper sine_shaper_;
  IFloat fader_ {0_f};
  IFloat warp_ {0_f};
  IFloat twist_ {0_f};
  IFloat mod_ {0_f};

  // simple linear piecewise function: 0->1, 0.25->1, 0.5->0
  static f antialias(f factor) {
    return (2_f - 4_f * factor).clip(0_f,1_f);
  }

public:
  template<TwistMode twist_mode, WarpMode warp_mode>
  static f Process(Phasor& ph, SineShaper& sh, u0_32 freq, u0_16 mod, f twist_amount, f warp_amount) {
    u0_32 phase = ph.Process(freq);
    phase += u0_32(mod);
    phase = Distortion::twist<twist_mode>(phase, twist_amount);

    s1_15 sine = twist_mode == FEEDBACK ?
      sh.Process(phase, u0_16(twist_amount)) :
      sh.Process(phase);

    return Distortion::warp<warp_mode>(sine, warp_amount);
  }

  template<TwistMode twist_mode, WarpMode warp_mode, int block_size>
  void Process(f const freq,
               Buffer<f, block_size>& twist,
               Buffer<f, block_size>& warp,
               f fade, f const amplitude,
               Buffer<f, block_size>& modulation,
               Buffer<u0_16, block_size>& mod_in,
               Buffer<u0_16, block_size>& mod_out,
               Buffer<f, block_size>& sum_output) {
    f aliasing_factor = freq;
    fade *= antialias(aliasing_factor);

    u0_32 const fr = u0_32(freq);
    Phasor ph = phasor_;
    SineShaper sh = sine_shaper_;
    IFloat fd = fader_;
    fd.set(fade, block_size);

    for (auto [sum, m_in, m_out, w, t, m] :
           zip(sum_output, mod_in, mod_out, warp, twist, modulation)) {
      f sample = Process<twist_mode, warp_mode>(ph, sh, fr, m_in, t, w);
      sample *= fd.next();
      // TODO comprendre +1
      m_out += u0_16((sample + 1_f) * m);
      sum += sample * amplitude;
    }

    phasor_ = ph;
    sine_shaper_ = sh;
    fader_ = fd;
  }
};

struct FrequencyPair { f freq1, freq2, crossfade; };

struct FrequencyState {
  OnePoleLp freq1, freq2, crossfade;
  void Process(f coef, FrequencyPair p) {
    freq1.Process(coef, p.freq1);
    freq2.Process(coef, p.freq2);
    crossfade.Process(coef, p.crossfade);
  }
};

class OscillatorPair : Nocopy {
  Oscillator osc_[2];
  FrequencyState freq;

public:

  template<TwistMode twist_mode, WarpMode warp_mode, int block_size>
  void Process(FrequencyPair new_freq,
               bool frozen,
               f crossfade_factor,
               Buffer<f, block_size>& twist,
               Buffer<f, block_size>& warp,
               Buffer<f, block_size>& modulation,
               f const amplitude,
               Buffer<u0_16, block_size>& mod_in, Buffer<u0_16, block_size>& mod_out,
               Buffer<f, block_size>& sum_output) {

    f coef = frozen ? 0_f : 0.1_f;
    freq.Process(coef, new_freq);

    f crossfade = freq.crossfade.state();

    // shape crossfade so notes are easier to find
    // crossfade = Math::fast_raised_cosine(crossfade);
    crossfade = Signal::crop(crossfade_factor, crossfade);

    f fade1 = 1_f - crossfade;
    f fade2 = crossfade;

    // mod_out is accumulated in the two calls, so we need to zero it here
    mod_out.fill(0._u0_16);
    osc_[0].Process<twist_mode, warp_mode>(freq.freq1.state(), twist, warp,
                                           fade1, amplitude, modulation,
                                           mod_in, mod_out, sum_output);
    osc_[1].Process<twist_mode, warp_mode>(freq.freq2.state(), twist, warp,
                                           fade2, amplitude, modulation,
                                           mod_in, mod_out, sum_output);
  }
};
