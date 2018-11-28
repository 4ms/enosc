

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

struct PolypticOscillator : Nocopy {
  Oscillator osc_;

  void Process(Float freq, Float *out, int size) {
    debug.set(3, true);
    while(size--) {
      s1_15 s = osc_.Process(u0_32(freq), u0_16(0.0_f));
      *out = s.to_float();
      out++;
    }
    debug.set(3, false);
  }

  void Process(Frame in, Frame out, int size) {
  }
};
