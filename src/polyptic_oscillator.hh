

class Oscillator {
  u0_32 phase_ = u0_32::of_repr(Random::Word());
  s1_15 history_ = 0._s1_15;
  IOnePoleLp<s1_15, 2> lp_;
  IOnePoleLp<u0_32, 3> lp_freq_;
  IOnePoleLp<u0_16, 5> lp_fb_;
public:

  s1_15 Process(u0_32 freq, u0_16 feedback) {
    lp_freq_.Process(freq, &freq);
    lp_fb_.Process(feedback, &feedback);
    s1_31 fb = history_ * feedback.to_signed();
    u0_32 phase = phase_;
    phase += freq + fb.to_unsigned() + u0_32(feedback);
    s1_15 sample = Data::short_sine.interpolate(phase).movl<15>();
    phase_ += freq;
    lp_.Process(sample, &history_);
    return sample;
  }
};

class Oscillators {
  Oscillator osc_;

public:
  void Process(f freq, f *out1, f *out2, int size) {
    debug.set(3, true);
    while(size--) {
      s1_15 s = osc_.Process(u0_32(freq), u0_16(0.0_f));
      *out1 = s.to_float();
      out1++;
    }
    debug.set(3, false);
  }
};

struct PolypticOscillator : Nocopy {
  Oscillators oscs_;

  void Process(Frame *in, Frame *out, int size) {
    f out1[size];
    f out2[size];

    f freq = 0.01_f;
    oscs_.Process(freq, out1, out2, size);

    for(f *o1=out1, *o2=out2; size--;) {
      out->l = s1_15(*o1);
      out->r = s1_15(*o2);
      out++; o1++; o2++;
    }
  }
};
