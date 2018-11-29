#include "dsp.hh"

class Phasor {
  u0_32 phase_ = u0_32::of_repr(Random::Word());
public:
  u0_32 Process(u0_32 freq) {
    phase_ += freq;
    return phase_;
  }
};

class SineShaper {
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

class Oscillator {
  Phasor phasor_;
  SineShaper shaper_;
public:
  s1_15 Process(u0_32 freq, u0_16 feedback) {
    u0_32 phase = phasor_.Process(freq);
    return shaper_.Process(phase, feedback);
  }
};

class Oscillators {
  Oscillator osc_[kNumOsc];

  s1_15 crush(s1_15 sample, f amount) {
    amount *= 15_f;
    f integral = amount.integral();
    s1_15 fractional = s1_15(amount.fractional() * 2_f - 1_f);
    if (sample < fractional) integral++;
    sample = sample.div2(integral.repr()).mul2(integral.repr());
    return sample;
  }

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
    for (int i=0; i<kNumOsc; i++) {
      oc = !oc;
      pitch += spread;
      pitch += detune;
      f freq = Freq::of_pitch(pitch).repr();
      for (f *o1=out1, *o2=out2; o1<out1+size; o1++, o2++) {
        s1_15 sample = osc_[i].Process(u0_32(freq), u0_16(twist));
        sample = crush(sample, warp);
        f t = sample.to_float();
        if (oc) *o1 += t; else *o2 += t;
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
      f s1 = *o1 / f(kNumOsc/2); // /2 because only half the voices
      f s2 = *o2 / f(kNumOsc/2);
      out->l = s1_15(s1);
      out->r = s1_15(s2);
      out++; o1++; o2++;
    }
  }
};
