#include <initializer_list>

#include "numtypes.hh"

using index = u32;

template<class T>
class Buffer {
  const T *data_;
  index size_;
public:
  constexpr Buffer(std::initializer_list<T> data) :
    data_(data.begin()), size_(data.size()) {};

  constexpr index size() const {return size_;}
  constexpr T operator[](index idx) const { return data_[idx.repr()]; }

  // zero-order hold
  constexpr T operator[](f const phase) const {
    phase *= (size()-1_u32).to_float();
    index integral = index(phase);
    Float a = data_[integral.repr()];
    return a;
  }

  constexpr T interpolate(f phase) const {
    phase *= (size()-1_u32).to_float();
    u32 integral = u32(phase);
    f fractional = phase - integral.to_float();
    T a = data_[integral.repr()];
    T b = data_[(integral+1_u32).repr()];
    return a + (b - a) * fractional;
  }

  // TODO generalize the 10 and the output type (fixed to s1_15 for now)
  constexpr T interpolate(u0_32 const phase) const {
    u10_22 p = phase.shift_right<10>();
    u32_0 integral = p.integral();
    T a = data_[(integral).repr()];
    T b = data_[(integral+1_u32).repr()];
    u0_32 fractional = p.fractional();
    s1_15 frac = fractional.to_narrow<0,16>().to<SIGNED>().shift_right<1>();
    return a + (b - a) * frac;
  }

};
