#include "dsp.hh"

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
  Oscillator osc_[kNumOsc];

public:
  void Process(Parameters &params, f *out1, f *out2, int size) {
    std::fill(out1, out1+size, 0_f);
    std::fill(out2, out2+size, 0_f);

    f pitch = params.pitch;
    f spread = params.spread;
    f feedback = params.warp;

    bool oc = false;
    for (int i=0; i<kNumOsc; i++) {
      oc = !oc;
      pitch += spread;
      f freq = Freq::of_pitch(pitch).repr();
      for (f *o1=out1, *o2=out2; o1<out1+size; o1++, o2++) {
        s1_15 s = osc_[i].Process(u0_32(freq), u0_16(feedback));
        if (oc)
          *o1 += s.to_float() / f(kNumOsc); // TODO: optimize scaling
        else
          *o2 += s.to_float() / f(kNumOsc); // TODO: optimize scaling
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
      out->l = s1_15(*o1);
      out->r = s1_15(*o2);      // TODO o2
      out++; o1++; o2++;
    }
  }
};
