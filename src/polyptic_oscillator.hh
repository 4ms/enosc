#include <algorithm>

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
};

class Oscillator : Nocopy {
  Phasor phasor_;
  SineShaper shaper_;

  // TODO optimize
  static f crush(s1_15 sample, f amount) {
    amount *= 15_f;
    f integral = amount.integral();
    s1_15 fractional = s1_15(amount.fractional() * 2_f - 1_f);
    if (sample < fractional) integral++;
    sample = sample.div2(integral.repr()).mul2(integral.repr());
    return sample.to_float();
  }

  // TODO optimize
  static f cheby(s1_15 x, f amount) {
    amount *= (Data::cheby.size() - 1_u32).to_float();
    index idx = index(amount);
    f frac = amount.fractional();
    u0_16 phase = x.to_unsigned_scale();
    f s1 = Data::cheby[idx].interpolate(phase);
    f s2 = Data::cheby[idx+1_u32].interpolate(phase);
    return Math::crossfade(s1, s2, frac);
  }

  static f fold(s1_15 x, f amount) {
    return x.to_float() * amount;
  }

public:
  template<TwistMode twist_mode, WarpMode warp_mode>
  f Process(u0_32 freq, f twist, f warp) {
    u0_32 phase = phasor_.Process(freq);

    f feedback = 0_f;
    if (twist_mode == FEEDBACK) {
      feedback = twist;
    }

    s1_15 sine = shaper_.Process(phase, u0_16(feedback));
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
};

class Oscillators : Nocopy {
  Oscillator osc_[kNumOsc];

public:
  void Process(Parameters &params, f *out1, f *out2, int size) {
    std::fill(out1, out1+size, 0_f);
    std::fill(out2, out2+size, 0_f);

    f pitch = params.pitch;
    f spread = params.spread;
    f twist = params.twist.value;
    f warp = params.warp.value;
    f detune = params.detune;

    bool oc = false;

    f (Oscillator::*process)(u0_32, f, f);

    if (params.twist.mode == FEEDBACK &&
        params.warp.mode == CRUSH) {
      process = &Oscillator::Process<FEEDBACK, CRUSH>;
    } else if (params.twist.mode == FEEDBACK &&
               params.warp.mode == CHEBY) {
      process = &Oscillator::Process<FEEDBACK, CHEBY>;
    } else if (params.twist.mode == FEEDBACK &&
               params.warp.mode == FOLD) {
      process = &Oscillator::Process<FEEDBACK, FOLD>;
    } else if (params.twist.mode == PULSAR &&
        params.warp.mode == CRUSH) {
      process = &Oscillator::Process<PULSAR, CRUSH>;
    } else if (params.twist.mode == PULSAR &&
               params.warp.mode == CHEBY) {
      process = &Oscillator::Process<PULSAR, CHEBY>;
    } else if (params.twist.mode == PULSAR &&
               params.warp.mode == FOLD) {
      process = &Oscillator::Process<PULSAR, FOLD>;
    } else if (params.twist.mode == DECIMATE &&
        params.warp.mode == CRUSH) {
      process = &Oscillator::Process<DECIMATE, CRUSH>;
    } else if (params.twist.mode == DECIMATE &&
               params.warp.mode == CHEBY) {
      process = &Oscillator::Process<DECIMATE, CHEBY>;
    } else if (params.twist.mode == DECIMATE &&
               params.warp.mode == FOLD) {
      process = &Oscillator::Process<DECIMATE, FOLD>;
    }


    for (int i=0; i<kNumOsc; i++) {
      pitch += spread;
      pitch += detune;
      f freq = Freq::of_pitch(pitch).repr();
      oc = !oc;
      f *output = oc ? out1 : out2;
      for (f *out = output; out<output+size; out++) {
        *out += (osc_[i].*process)(u0_32(freq), twist, warp);
      }
    }
  }
};

struct PolypticOscillator : Nocopy {
  Oscillators oscs_;

  void Process(Parameters &params, Frame *in, Frame *out, int size) {
    f out1[size];
    f out2[size];

    oscs_.Process(params, out1, out2, size);

    for(f *o1=out1, *o2=out2; size--;) {
      f s1 = *o1 / f(kNumOsc); // TODO: kNumOsc/2 because only half the voices
      f s2 = *o2 / f(kNumOsc);
      out->l = s1_15(s1);
      out->r = s1_15(s2);
      out++; o1++; o2++;
    }
  }
};
