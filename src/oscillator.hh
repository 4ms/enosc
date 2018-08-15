#include "numtypes.hh"
#include "data.hh"
#include "dsp.hh"

// Float version

class FOscillator {
  Float phase_ = 0_f;
  Float history_ = 0_f;
  OnePoleLp lp_;
public:
  Float Process(Float freq, Float feedback_amount) {
    f phase = phase_;
    phase += freq
      + history_ * feedback_amount
      + 10.0_f;
    phase = phase.fractional();
    f sample = Data::sine.interpolate(phase);
    lp_.Process(0.25_f, sample, &history_);
    phase_ += freq;
    return sample;
  }
};

template<int kNumOsc>
class FOscillators {
  FOscillator osc_[kNumOsc];
public:
  Float Process(Float freq) {
    Float output = 0_f;
    for(int i=0; i<kNumOsc; i++) {
      output += osc_[i].Process(freq, 0.4_f);
      freq += 0.001_f;
    }
    return output / Float(kNumOsc);
  }
};

// Integer version

class IOscillator {
  u0_32 phase_ = 0._u0_32;
  s1_15 history_ = 0._s1_15;
  IOnePoleLp<s1_15, 1> lp_;
public:
  s1_15 Process(u0_32 freq, u0_16 feedback) {
    // TODO multiplication
    // s1_31 fb = (history_ * feedback.shift_right<1>().to<SIGNED>()).to<1,31>();
    // s1_31 fb = s1_31::of_repr(history_.repr() * feedback.repr());
    s1_31 fb = s1_31::of_repr(history_.repr() * feedback.shift_right<1>().to<SIGNED>().repr());
    u0_32 phase = phase_;
    phase += freq + fb.shift_left<1>().to<UNSIGNED>();
    s1_15 sample = Data::short_sine.interpolate(phase).shift_left<15>();
    phase_ += freq;
    lp_.Process(sample, &history_);
    return sample;
  }
};

template<int kNumOsc>
class IOscillators {
  IOscillator osc_[kNumOsc];
public:
  s1_15 Process(u0_32 freq) {
    s17_15 sum = 0._s17_15;
    for(int i=0; i<kNumOsc; i++) {
      s1_15 x = osc_[i].Process(freq, 0.4_u0_16);
      s17_15 y(x);
      sum += y;
      freq += 0.001001_u0_32;
    }
    s1_15 output = (sum / kNumOsc).to_sat<1, 15>();
    return output;
  }
};
