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
    f x = sample.to_float();
    union { float a; uint32_t b; } t = {x.repr()};
    t.b ^= (uint32_t)((t.b & ((1 << 23)-1)) * amount.repr());
    // t.b ^= (int32_t)(((1<<23)-1) * amount.repr());
    return f(t.a);
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
    u0_32 phase = phasor_.Process(freq);

    f feedback = 0_f;
    if (twist_mode == FEEDBACK) {
      feedback = twist;
    } else if (twist_mode == PULSAR) {
      phase = pulsar(phase, twist);
    } else if (twist_mode == DECIMATE) {
      phase = decimate(phase, twist);
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

  template<TwistMode twist_mode, WarpMode warp_mode>
  void Process(u0_32 freq, f twist, f warp, f amplitude, f *output, int size) {
      for (f *out = output; out<output+size; out++) {
        f sample = Process<twist_mode, warp_mode>(u0_32(freq), twist, warp);
        *out += sample * amplitude;
      }
  }
};

class Oscillators : Nocopy {
  Oscillator osc_[kNumOsc];

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
  void Process(Parameters &params, f *out1, f *out2, int size) {
    std::fill(out1, out1+size, 0_f);
    std::fill(out2, out2+size, 0_f);

    void (Oscillator::*process)(u0_32, f, f, f, f*, int);

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

    // scaling and response of common parameters
    f twist = params.twist.value;
    f warp = params.warp.value;

    f root = params.root;
    f pitch = params.pitch;
    f spread = params.spread;
    f detune = params.detune;
    f tilt = params.tilt;

    f amplitude = 1_f;
    f amplitudes = 0_f;

    for (int i=0; i<kNumOsc; i++) {
      f freq = Freq::of_pitch(pitch).repr();

      bool voice;
      if (params.stereo_mode == ALTERNATE) {
        voice = i&1;
      } else if (params.stereo_mode == SPLIT) {
        voice = i<=kNumOsc/2;
      } else {
        voice = i==0;
      }

      // antialias
      f aliasing_factor = freq;
      amplitude *= antialias(aliasing_factor);
      amplitudes += amplitude;

      f *output;
      if (voice) {
        output = out1;
      } else {
        output = out2;
      }

      (osc_[i].*process)(u0_32(freq), twist, warp, amplitude, output, size);

      pitch += spread;
      pitch += detune;
      amplitude *= tilt;
    }

    f atten = 1_f / amplitudes;
    for(f *o1=out1, *o2=out2; size--;) {
      *o1 *= atten;
      *o2 *= atten;
      o1++; o2++;
    }
  }
};

struct PolypticOscillator : Nocopy {
  Oscillators oscs_;

  void Process(Parameters &params, Frame *in, Frame *out, int size) {
    f buffer[2][size];

    oscs_.Process(params, buffer[0], buffer[1], size);

    for(f *o1=buffer[0], *o2=buffer[1]; size--;) {
      f s1 = *o1;
      f s2 = *o2;
      out->l = s1_15(s1);
      out->r = s1_15(s2);
      out++; o1++; o2++;
    }
  }
};
