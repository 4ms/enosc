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
  void Process(u0_32 freq, f twist, f warp, f amplitude, Block<f> output) {
    for (f &out : output) {
      f sample = Process<twist_mode, warp_mode>(u0_32(freq), twist, warp);
      out += sample * amplitude;
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
  void Process(f freq1, f freq2, f phase, f twist, f warp, f amplitude, Block<f> output) {
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

class Oscillators : Nocopy {
  DoubleOscillator osc_[kNumOsc];

  static bool pick_output(StereoMode mode, int i) {
    return
      mode == ALTERNATE ? i&1 :
      mode == SPLIT ? i<=kNumOsc/2 :
      i == 0;
  }

  class AmplitudeAccumulator {
    f amplitude = 1_f;
    f amplitudes = 0_f;
    f tilt;
  public:
    AmplitudeAccumulator(f t) : tilt(t) {}
    f Next() {
      f r = amplitude;
      amplitudes += amplitude;
      amplitude *= tilt;
      return r;
    }
    f Sum() { return amplitudes; }
  };

  class FrequencyAccumulator {
    f root;
    f pitch;
    f spread;
    f detune;
    f detune_accum = 0_f;
  public:
    FrequencyAccumulator(f r, f p, f s, f d) : root(r), pitch(p), spread(s), detune(d) {}
    void Next(f& freq1, f& freq2, f& phase) {
      // quantize pitch
      int32_t integral = (root / 6_f).floor();
      f p1 = f(integral) * 6_f;
      f p2 = p1 + 6_f;
      phase = (root - p1) / (p2 - p1);

      p1 += pitch + detune_accum;
      p2 += pitch + detune_accum;

      if (integral & 1) {
        phase = 1_f - phase;
        f tmp = p1;
        p1 = p2;
        p2 = tmp;
      }

      freq1 = Freq::of_pitch(p1).repr();
      freq2 = Freq::of_pitch(p2).repr();

      root += spread;
      detune_accum += detune;
    }
  };

  using processor_t = void (DoubleOscillator::*)(f, f, f, f, f, f, Block<f>);

  processor_t choose_processor(TwistMode t, WarpMode m) {
    return
      t == FEEDBACK && m == CRUSH ? &DoubleOscillator::Process<FEEDBACK, CRUSH> :
      t == FEEDBACK && m == CHEBY ? &DoubleOscillator::Process<FEEDBACK, CHEBY> :
      t == FEEDBACK && m == FOLD ? &DoubleOscillator::Process<FEEDBACK, FOLD> :
      t == PULSAR && m == CRUSH ? &DoubleOscillator::Process<PULSAR, CRUSH> :
      t == PULSAR && m == CHEBY ? &DoubleOscillator::Process<PULSAR, CHEBY> :
      t == PULSAR && m == FOLD ? &DoubleOscillator::Process<PULSAR, FOLD> :
      t == DECIMATE && m == CRUSH ? &DoubleOscillator::Process<DECIMATE, CRUSH> :
      t == DECIMATE && m == CHEBY ? &DoubleOscillator::Process<DECIMATE, CHEBY> :
      t == DECIMATE && m == FOLD ? &DoubleOscillator::Process<DECIMATE, FOLD> :
      NULL;
  }

public:
  void Process(Parameters &params, Block<f> out1, Block<f> out2) {
    out1.fill(0_f);
    out2.fill(0_f);

    // scaling and response of common parameters
    f twist = params.twist.value;
    f warp = params.warp.value;

    AmplitudeAccumulator amplitude {params.tilt};
    FrequencyAccumulator frequency {params.root, params.pitch,
                                    params.spread, params.detune};

    processor_t process = choose_processor(params.twist.mode, params.warp.mode);

    for (int i=0; i<kNumOsc; i++) {
      // antialias
      f freq1, freq2, phase;
      frequency.Next(freq1, freq2, phase);

      f amp = amplitude.Next();
      Block<f> out = pick_output(params.stereo_mode, i) ? out1 : out2;
      (osc_[i].*process)(freq1, freq2, phase, twist, warp, amp, out);
    }

    f atten = 1_f / amplitude.Sum();

    f *begin1 = out1.begin();
    for (f& o2 : out2) {
      f& o1 = *begin1;
      o1 *= atten;
      o2 *= atten;
      begin1++;
    }
  }
};

struct PolypticOscillator : Nocopy {
  Oscillators oscs_;

  void Process(Parameters &params, Block<Frame> out) {
    f buffer[2][out.size()];
    Block<f> out1 {buffer[0], out.size()};
    Block<f> out2 {buffer[1], out.size()};

    oscs_.Process(params, out1, out2);

    f *b1 = out1.begin();
    f *b2 = out2.begin();
    for (Frame& o : out) {
      o.l = s1_15(*b1);
      o.r = s1_15(*b2);
      b1++; b2++;
    }
  }
};
