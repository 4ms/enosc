#include "numtypes.hh"
#include "data.hh"

// x between -1 and +1
Float FasterSine(Float x) {
  x = (x * 2_f) - 1_f;
  return 4_f * (x - x * x.abs());
}

// x between -1 and +1
Float FastSine(Float x) {
  Float y = FasterSine(x);
  y = 0.225_f * (y * y.abs() - y) + y;
  return y;
}

// LUT + ZOH
Float sine1(Float phase) {
  phase *= Float(Data::sine.size()-1);
  int integral = s32(phase).repr();
  Float a = Data::sine[integral];
  return a;
}

// LUT + linear interpolation
Float sine2(Float phase) {
  phase *= Float(Data::sine.size()-1);
  u32 integral = u32(phase);
  Float fractional = phase - integral.to_float();
  Float a = Data::sine[integral.repr()];
  Float b = Data::sine[integral.repr()+1];
  return a + (b - a) * fractional;
}

// TODO: fix sine2 above (pb with long long int in Numtypes)
Float sine2_alamain(Float phase) {
  phase *= Float(Data::sine.size()-1);
  u32 integral = u32(DANGER, static_cast<uint32_t>(phase.repr()));
  Float fractional = phase - integral.to_float();
  Float a = Data::sine[integral.repr()];
  Float b = Data::sine[(integral+1_u32).repr()];
  return a + (b - a) * f(fractional);
}

// polynomial
Float sine3(Float x) {
  return FastSine(x);
}

class FOscillator {
  Float phase_ = 0_f;
public:
  Float Process(Float freq) {
    phase_ += freq;
    if (phase_ > 1_f) phase_--;
    return sine2_alamain(phase_);
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
    return sine2_alamain(phase_.to_float());
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
      freq += 0.001_u0_32;      // TODO mult
    }
    return output / Float(kNumOsc);
  }
};

// Integer version

// x between -1 and +1
s1_15 FasterSinei(u0_32 x) {
  s1_31 y = x.to_wrap<SIGNED, 1, 31>() - 1._s1_31;
  s1_15 z = (y * 2).to_narrow<1,15>();
  z -= z * z.abs();
  z *= 2;
  z = z.add_sat(z);
  return z;
}

// x between -1 and +1
s1_15 FastSinei(u0_32 x) {
  s1_15 y = FasterSinei(x);
  y = (y * y.abs() - y) * 0.4_s1_15 / 0.9_s1_15 + y;
  return y;
}

// polynomial
s1_15 sine3i(u0_32 x) {
  return FasterSinei(x);
}

// LUT + linear interpolation
s1_15 sine2i(u0_32 const phase) {
  u10_22 p = phase.shift_right<10>();
  u32_0 integral = p.integral();
  s1_15 a = s1_15(DANGER, Data::short_sine[integral.repr()]); // TODO Data
  s1_15 b = s1_15(DANGER, Data::short_sine[integral.repr()+1]);
  u0_32 fractional = p.fractional();
  return a + (b - a) * fractional.to_wrap<SIGNED, 1, 15>();
}

class IOscillator {
  u0_32 phase_ = 0._u0_32;
public:
  s1_15 Process(u0_32 freq) {
    phase_ += freq;
    return sine2i(phase_);
  }
};

template<int kNumOsc>
class IOscillators {
  IOscillator osc_[kNumOsc];
public:
  s1_15 Process(u0_32 freq) {
    s17_15 sum = 0._s17_15;
    for(int i=0; i<kNumOsc; i++) {
      s1_15 x = osc_[i].Process(freq);
      s17_15 y(x);
      sum += y;
      freq += 0.001_u0_32;      // TODO mult
    }
    s1_15 output = (sum / kNumOsc).to_sat<1, 15>();
    return output;
  }
};
