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
  IFloat fade_, twist_, warp_, modulation_, amplitude_;

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
               f twist,
               f warp,
               f modulation,
               f fade, f const amplitude,
               Buffer<u0_16, block_size>& mod_in,
               Buffer<u0_16, block_size>& mod_out,
               Buffer<f, block_size>& sum_output) {

    fade = Antialias::freq(freq, fade);
    modulation = Antialias::modulation(freq, modulation);
    twist = Antialias::twist<twist_mode>(freq, twist);
    warp = Antialias::warp<warp_mode>(freq, warp);

    twist_.set(twist, block_size);
    warp_.set(warp, block_size);
    modulation_.set(modulation, block_size);
    amplitude_.set(amplitude, block_size);

    u0_32 const fr = u0_32(freq);
    Phasor ph = phasor_;
    SineShaper sh = sine_shaper_;
    IFloat fd=fade_, am=amplitude_, md=modulation_, tw=twist_, wa=warp_;

    fd.set(fade, block_size);

    for (auto [sum, m_in, m_out] : zip(sum_output, mod_in, mod_out)) {
      f sample = Process<twist_mode, warp_mode>(ph, sh, fr, m_in, tw.next(), wa.next());
      sample *= fd.next();
      // TODO comprendre +1
      m_out += u0_16((sample + 1_f) * md.next());
      sum += sample * am.next();
    }

    phasor_ = ph;
    sine_shaper_ = sh;
    fade_ = fd;
    amplitude_ = am;
    modulation_ = md;
    twist_ = tw;
    warp_ = wa;
  }
};

struct FrequencyPair { f freq1, freq2, crossfade; };

class FrequencyState {
  OnePoleLp freq1_, freq2_, crossfade_;
public:
  FrequencyPair Process(f coef, FrequencyPair p) {
    freq1_.Process(coef, p.freq1);
    freq2_.Process(coef, p.freq2);
    crossfade_.Process(coef, p.crossfade);
    return {freq1_.state(), freq2_.state(), crossfade_.state()};
  }
};

template<int block_size>
class OscillatorPair : Nocopy {
  Oscillator osc_[2];
  FrequencyState freq_;

public:

  using processor_t = void (Oscillator::*)(f const freq,
                                           f twist,
                                           f warp,
                                           f modulation,
                                           f fade, f const amplitude,
                                           Buffer<u0_16, block_size>& mod_in,
                                           Buffer<u0_16, block_size>& mod_out,
                                           Buffer<f, block_size>& sum_output);

  static processor_t pick_processor(TwistMode t, WarpMode m) {
    static processor_t tab[3][3] = {
      &Oscillator::Process<FEEDBACK, FOLD, block_size>,
      &Oscillator::Process<FEEDBACK, CHEBY, block_size>,
      &Oscillator::Process<FEEDBACK, SEGMENT, block_size>,
      &Oscillator::Process<PULSAR, FOLD, block_size>,
      &Oscillator::Process<PULSAR, CHEBY, block_size>,
      &Oscillator::Process<PULSAR, SEGMENT, block_size>,
      &Oscillator::Process<CRUSH, FOLD, block_size>,
      &Oscillator::Process<CRUSH, CHEBY, block_size>,
      &Oscillator::Process<CRUSH, SEGMENT, block_size>,
    };
    return tab[t][m];
  }

  void Process(TwistMode twist_mode, WarpMode warp_mode, FrequencyPair freq,
               bool frozen,
               f crossfade_factor,
               f twist,
               f warp,
               f modulation,
               f const amplitude,
               Buffer<u0_16, block_size>& mod_in, Buffer<u0_16, block_size>& mod_out,
               Buffer<f, block_size>& sum_output) {

    f coef = frozen ? 0_f : 1.0_f;
    auto [freq1, freq2, crossfade] = freq_.Process(coef, freq);

    processor_t process = pick_processor(twist_mode, warp_mode);

    // shape crossfade so notes are easier to find
    // crossfade = Math::fast_raised_cosine(crossfade);
    crossfade = Signal::crop(crossfade_factor, crossfade);

    f fade1 = 1_f - crossfade;
    f fade2 = crossfade;

    // mod_out is accumulated in the two calls, so we need to zero it here
    mod_out.fill(0._u0_16);
    (osc_[0].*process)(freq1, twist, warp,
                       modulation, fade1, amplitude,
                       mod_in, mod_out, sum_output);
    (osc_[1].*process)(freq2, twist, warp,
                       modulation, fade2, amplitude,
                       mod_in, mod_out, sum_output);
  }
};
