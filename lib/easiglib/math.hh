#include "numtypes.hh"
#include "data.hh"

#pragma once

struct Math {
  static constexpr f pi = f(3.14159265358979323846264338327950288);


  // [-1; +1] --> [-1; +1]
  static constexpr f faster_sine(f x) {
    x = (x * 2_f) - 1_f;
    return 4_f * (x - x * x.abs());
  }

  // [-1; +1] --> [-1; +1]
  static constexpr s1_15 faster_sine(u0_32 x) {
    s1_31 y = x.to_wrap<SIGNED, 1, 31>() - 1._s1_31;
    s1_15 z = (y * 2).to_narrow<1,15>();
    z -= (z * z.abs()).shiftr<16>();
    z *= 2;
    z = z.add_sat(z);
    return z;
  }

  // [-1; +1] --> [-1; +1]
  static constexpr f fast_sine(f x) {
    f y = faster_sine(x);
    y = 0.225_f * (y * y.abs() - y) + y;
    return y;
  }

  static constexpr f fast_tanh(f x) {
    return x * (27_f + x * x) / (27_f + 9_f * x * x);
  }

  static constexpr f fast_exp2(f x) {
    static_assert(is_power_of_2(Data::exp2_u0_23.size().repr()), "");
    constexpr int BITS = Log2<Data::exp2_u0_23.size().repr()>::val;

    typedef union {
      f f_repr;
      u32 i_repr;
      struct {
        uint32_t mantisa : 23;
        uint32_t exponent : 8;
        uint32_t sign : 1;
      } parts;
    } float_cast;

    u32 i = u32(((x + 127_f) * f(1 << 23)));
    float_cast u = {.i_repr = i};

    u.parts.mantisa = Data::exp2_u0_23[index::of_repr(u.parts.mantisa >> 23-BITS)].repr();
    return u.f_repr;
  }
};
