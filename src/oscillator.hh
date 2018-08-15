#include "numtypes.hh"
#include "data.hh"

// Float version

class FOscillator {
  Float phase_ = 0_f;
public:
  Float Process(Float freq) {
    phase_ += freq;
    if (phase_ > 1_f) phase_--;
    return Data::sine.interpolate(phase_);
  }
};

template<int kNumOsc>
class FOscillators {
  FOscillator osc_[kNumOsc];
public:
  Float Process(Float freq) {
    Float output = 0_f;
    for(int i=0; i<kNumOsc; i++) {
      output += osc_[i].Process(freq);
      freq += 0.001_f;
    }
    return output / Float(kNumOsc);
  }
};

// Mixed version: fixed phase, floating output

class IFOscillator {
  u0_32 phase_ = 0._u0_32;
public:
  Float Process(u0_32 freq) {
    phase_ += freq;
    return Data::sine.interpolate(phase_.to_float());
  }
};

template<int kNumOsc>
class IFOscillators {
  IFOscillator osc_[kNumOsc];
public:
  Float Process(u0_32 freq) {
    Float output = 0_f;
    for(int i=0; i<kNumOsc; i++) {
      output += osc_[i].Process(freq);
      freq += 0.001_u0_32;
    }
    return output / Float(kNumOsc);
  }
};

// Integer version

class IOscillator {
  u0_32 phase_ = 0._u0_32;
  s1_31 history_ = 0._s1_31;
public:
  s1_15 Process(s1_31 freq, u0_16 feedback) {
    s1_31 fb = history_ * feedback.shift_left<1>().to<SIGNED>();
    phase_ += (freq + fb).shift_left<1>().to<UNSIGNED>();
    s1_15 sample = Data::short_sine.interpolate(phase_).shift_left<15>();
    history_ = sample.to<1,31>();
    return sample;
  }
};

template<int kNumOsc>
class IOscillators {
  IOscillator osc_[kNumOsc];
public:
  s1_15 Process(s1_31 freq) {
    s17_15 sum = 0._s17_15;
    for(int i=0; i<kNumOsc; i++) {
      s1_15 x = osc_[i].Process(freq, 0.4_u0_16);
      s17_15 y(x);
      sum += y;
      freq += 0.001_s1_31;      // TODO mult
    }
    s1_15 output = (sum / kNumOsc).to_sat<1, 15>();
    return output;
  }
};
