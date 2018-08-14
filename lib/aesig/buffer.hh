#include <initializer_list>

#include "util.hh"
#include "numtypes.hh"

using index = u32;

template<class T, unsigned int SIZE>
class Buffer {
  const T *data_;
public:
  constexpr Buffer(std::initializer_list<T> data) :
    data_(data.begin()) {};

  constexpr index size() const {return index(SIZE);}
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

  constexpr T interpolate(u0_32 const phase) const {
    static_assert(is_power_of_2(SIZE-1),
                  "Integer interpolate supports only power-of-two-sized buffers");
    constexpr int bits = Log2<SIZE>::val;
    Fixed<UNSIGNED, bits, 32-bits> p = phase.shift_right<bits>();
    u32_0 integral = p.integral();
    T a = data_[(integral).repr()];
    T b = data_[(integral+1_u32).repr()];
    u0_32 fractional = p.fractional();
    s1_15 frac = fractional.to_narrow<0,16>().shift_right<1>().to<SIGNED>();
    return a + (b - a) * frac;
  }

};
