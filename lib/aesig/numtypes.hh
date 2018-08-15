#pragma once

#include <cstdint>
#include <cmath>
#include <cfloat>

#ifdef __arm__
  #include "stm32f4xx.h"
#endif

inline int libc_abs(int x) { return abs(x); }

/**************
 * 32-bits Floating Point
 **************/

// template here is a hack to be able to define min/max-val inside the
// class as static constexpr; otherwise it complains that type is incomplete
template<bool>
class FloatT {
  float val_;
public:
  using T = FloatT;
  explicit FloatT() { }
  explicit constexpr FloatT(float v) : val_(v) { }
  constexpr float repr() const { return val_; }
  constexpr T const operator+(const T y) const { return T(repr() + y.repr()); }
  constexpr T const operator-(const T y) const { return T(repr() - y.repr()); }
  constexpr T const operator-() const { return T(-repr()); }
  constexpr T const operator*(const T y) const { return T((repr() * y.repr())); }
  constexpr T const operator/(const T y) const { return T((repr()) / y.repr()); }

  constexpr bool const operator<(const T y) const { return repr() < y.repr(); }
  constexpr bool const operator>(const T y) const { return repr() > y.repr(); }
  constexpr bool const operator<=(const T y) const { return repr() <= y.repr(); }
  constexpr bool const operator>=(const T y) const { return repr() >= y.repr(); }
  constexpr bool const operator==(const T y) const { return repr() == y.repr(); }
  constexpr bool const operator!=(const T y) const { return repr() != y.repr(); }

  constexpr void operator+=(const T y) { val_ += y.repr(); }
  constexpr void operator-=(const T y) { val_ -= y.repr(); }
  constexpr void operator*=(const T y) { val_ = val_ * y.repr(); }
  constexpr void operator/=(const T y) { val_ = val_ / y.repr(); }
  constexpr void operator++() { ++val_; }
  constexpr void operator--() { --val_; }
  constexpr void operator++(int) { val_++; }
  constexpr void operator--(int) { val_--; }

  constexpr T const abs() const { return T(fabsf(val_)); }
  T const sqrt() const {
#ifdef __arm__
    float y;
      __asm("VSQRT.F32 %0,%1" : "=t"(y) : "t"(val_));
    return T(y);
#else
    return T(sqrtf(val_));
#endif
  }

  static constexpr T min_val = T(FLT_MIN);
  static constexpr T max_val = T(FLT_MIN);

  // clip
  constexpr T min(const T y) const { return *this < y ? *this : y; }
  constexpr T max(const T y) const { return *this < y ? y : *this; }
  constexpr T clip(const T x, const T y) const { return max(x).min(y); }
  constexpr T clip() const { return clip(T(-1.0f), T(1.0f)); }

  constexpr T fractional() const {
    return T(static_cast<float>(val_ - static_cast<int32_t>(val_)));
  }
};

using Float = FloatT<true>;
using f32 = Float;
using f = f32;


constexpr Float operator "" _f(long double f){ return Float(f); }
constexpr Float operator "" _f(unsigned long long int f){ return Float(f); }

/***************
 * 16-bits Floating Point
 ***************/

#ifdef __arm__

struct Float16 {
  Float16(Float x) : val_(x.repr()) { };
  Float to_float() { return Float(val_); }
  // unsigned short repr() { return t.repr; }
private:
  __fp16 val_;
};

#endif

/***************
 * Fixed-Point
 ***************/

struct dangerous { };
extern dangerous DANGER;

enum sign {
  UNSIGNED,
  SIGNED
};

template<int WIDTH, sign SIGN> struct Basetype;
// Wider should be twice as big as T
template<> struct Basetype<8, SIGNED> { using T = int8_t; using Wider = int16_t; };
template<> struct Basetype<8, UNSIGNED> { using T = uint8_t; using Wider = uint16_t; };
template<> struct Basetype<16, SIGNED> { using T = int16_t; using Wider = int32_t; };
template<> struct Basetype<16, UNSIGNED> { using T = uint16_t; using Wider = uint32_t; };
template<> struct Basetype<32, SIGNED> { using T = int32_t;
#ifndef __arm__
  using Wider = __int128;
#endif
};
template<> struct Basetype<32, UNSIGNED> { using T = uint32_t;
  #ifndef __arm__
  using Wider = unsigned __int128;
  #endif
};

template<sign SIGN, int INT, int FRAC>
class Fixed {
  static constexpr int WIDTH = INT + FRAC;

  using Base = typename Basetype<WIDTH, SIGN>::T;
  using T = Fixed<SIGN, INT, FRAC>;

  Base val_;

  template <typename A, int BITS>
  static constexpr A const saturate_integer(A x) {
    if (SIGN) {
      A vmin = -(1LL<<(BITS-1));
      A vmax = (1LL<<(BITS-1))-1;
      return x < vmin ? vmin : x > vmax ? vmax : x;
    } else {
      A vmin = 0;
      A vmax = (1ULL<<BITS)-1;
      return x < vmin ? vmin : x > vmax ? vmax : x;
    }
  }

#ifdef __arm__
  template <int BITS>
  T const saturate() const {
    static_assert(BITS > 0 && BITS < WIDTH, "Invalid bit count");
    if (SIGN) return T(DANGER, __SSAT(val_, BITS));
    else return T(DANGER, __USAT(val_, BITS));
  }
#else
  template <int BITS>
  constexpr T const saturate() const {
    static_assert(BITS > 0 && BITS < WIDTH, "Invalid bit count");
    return T(saturate_integer<Base, BITS>(y.val_));
  }
#endif

public:
  explicit constexpr Fixed(long double x) :
    val_(static_cast<long long int>((x * (long double)(1ULL << FRAC)))) { }

  explicit constexpr Fixed(Float x) :
    val_(static_cast<Base>((x * Float(1ULL << FRAC)).repr())) { }
  explicit constexpr Fixed(dangerous, Base x) : val_(x) {}

  static constexpr T of_int(unsigned long long int x) {
    return T(DANGER, x * (1ULL << FRAC));
  }

  template<int INT2, int FRAC2>
  explicit constexpr Fixed(Fixed<SIGN, INT2, FRAC2> const that) {
    Fixed<SIGN, INT, FRAC> y = that.template to<INT, FRAC>();
    val_ = y.repr();
  }

  static constexpr T min_val = T(DANGER, SIGN ? (Base)(1 << (WIDTH-1)) : 0);
  static constexpr T max_val = T(DANGER, SIGN ? (Base)(~(1 << (WIDTH-1))) : -1);
  static constexpr T increment = T(DANGER, 1);

  // Conversions:

  constexpr Base repr() const { return val_; }
  constexpr Float const to_float() const { return Float(repr()) / Float(1ULL << FRAC); }

  // promotion => no loss of information
  template <sign SIGN2, int INT2, int FRAC2>
  constexpr Fixed<SIGN2, INT2, FRAC2> const to() const {
    static_assert(SIGN2 == SIGN, "Conversion with different signs");
    static_assert(FRAC2 >= FRAC, "Conversion with possible loss of precision");
    static_assert(INT2 >= INT, "Conversion with possible wrapover");

    Base x = repr();
    // WARNING! possibly shifting negative integer
    return Fixed<SIGN2, INT2, FRAC2>(DANGER, (unsigned)repr() << (FRAC2 - FRAC));
  }

  // narrowing conversion => possible loss of precision
  template <int INT2, int FRAC2>
  constexpr Fixed<SIGN, INT2, FRAC2> const to_narrow() const {
    static_assert(FRAC2 < FRAC, "This is a promotion: use to()");
    static_assert(INT2 >= INT, "Conversion with possible wrapover");

    Base x = repr();
    // narrowing the fractional part => rounding
    return Fixed<SIGN, INT2, FRAC2>(DANGER, x >> (FRAC - FRAC2));
  }

  // TODO cleanup + test! this one has no safeguard
  template <sign SIGN2, int INT2, int FRAC2>
  constexpr Fixed<SIGN2, INT2, FRAC2> const to_wrap() const {
    if (FRAC2 >= FRAC) {
      return Fixed<SIGN2, INT2, FRAC2>(DANGER, (unsigned)val_ << (FRAC2 - FRAC));
    } else {
      return Fixed<SIGN2, INT2, FRAC2>(DANGER, val_ >> (FRAC - FRAC2));
    }
  }

  // TODO optimize for ARM: SAT/USAT instructions have built-in shift
  template <int INT2, int FRAC2>
  constexpr Fixed<SIGN, INT2, FRAC2> const to_sat() const {
    static_assert(INT2 < INT, "this is for saturation, use to()");
    Base x = saturate<WIDTH-(INT-INT2)>().repr();

    if (FRAC2 >= FRAC) {
      return Fixed<SIGN, INT2, FRAC2>(DANGER, (unsigned)x << (FRAC2 - FRAC));
    } else {
      return Fixed<SIGN, INT2, FRAC2>(DANGER, x >> (FRAC - FRAC2));
    }
  }

  template<sign SIGN2>
  constexpr Fixed<SIGN2, INT, FRAC> const to() const {
    if (SIGN==UNSIGNED && SIGN2==SIGNED) {
      return Fixed<SIGN2, INT, FRAC>(DANGER, (signed)val_ >> 1);
    } else if (SIGN==SIGNED && SIGN2==UNSIGNED) {
      return Fixed<SIGN2, INT, FRAC>(DANGER, (unsigned)val_);
    }
  }

  template<int INT2, int FRAC2>
  constexpr Fixed<SIGN, INT2, FRAC2> const to() const {
    return to<SIGN, INT2, FRAC2>();
  }

  template <int SHIFT>
  constexpr Fixed<SIGN, INT+SHIFT, FRAC-SHIFT> shift_right() const {
    return Fixed<SIGN, INT+SHIFT, FRAC-SHIFT>(DANGER, val_);
  }

  template <int SHIFT>
  constexpr Fixed<SIGN, INT-SHIFT, FRAC+SHIFT> shift_left() const {
    return Fixed<SIGN, INT-SHIFT, FRAC+SHIFT>(DANGER, val_);
  }

  // Operations:

  // in/decrement by the smallest amount possible in the representation
  constexpr T succ() const { return T(DANGER, repr()+1L); }
  constexpr T pred() const { return T(DANGER, repr()-1L); }

  constexpr T floor() const { return T(DANGER, repr() & ~((1ULL << FRAC) - 1ULL)); }
  constexpr T frac() const { return T(DANGER, repr() & ((1ULL << FRAC) - 1ULL)); }

  constexpr Fixed<SIGN, WIDTH, 0> integral() const {
    return to_narrow<WIDTH, 0>();
  }

  constexpr Fixed<SIGN, 0, WIDTH> fractional() const {
    return to_wrap<SIGN, 0, WIDTH>();
  }

  constexpr T const abs() const { return T(DANGER, libc_abs(val_)); }

  constexpr T operator-() const {
    static_assert(SIGN, "Prefix negation is invalid on unsigned data");
    return T(DANGER, -repr());
  }

  constexpr T operator+(T y) const { return T(DANGER, repr() + y.repr()); }
  constexpr T operator-(T y) const { return T(DANGER, repr() - y.repr()); }

  template <int INT2, int FRAC2>
  constexpr T operator*(Fixed<SIGN, INT2, FRAC2> y) const {
    static_assert(INT2+FRAC2 <= WIDTH, "Multiplier is too large");
    using Wider = typename Basetype<INT2+FRAC2, SIGN>::Wider;
    return T(DANGER, ((Wider)repr() * (Wider)y.repr()) >> FRAC2);
  }

  constexpr T operator*(Base y) const {
    return T(DANGER, repr() * y);
  }

  constexpr T operator/(T y) const {
    using Wider = typename Basetype<WIDTH, SIGN>::Wider;
    return T(DANGER, (static_cast<Wider>(repr()) << FRAC) / static_cast<Wider>(y.repr()));
  }

  constexpr T operator/(Base y) const {
    return T(DANGER, repr() / y);
  }

  template <int BITS>
  constexpr T div2() const {
    static_assert(BITS >= 0 && BITS < WIDTH, "Invalid bit count");
    return T(DANGER, repr() >> BITS);
  }

  constexpr void operator+=(T y) { val_ += y.repr(); }
  constexpr void operator-=(T y) { val_ -= y.repr(); }
  constexpr void operator*=(T y) { val_ = (*this * y).repr(); }
  constexpr void operator/=(T y) { val_ = (*this / y).repr(); }
  constexpr void operator*=(Base y) { val_ = (*this * y).repr(); }
  constexpr void operator/=(Base y) { val_ = (*this / y).repr(); }

  constexpr bool const operator<(const T y) const { return repr() < y.repr(); }
  constexpr bool const operator>(const T y) const { return repr() > y.repr(); }
  constexpr bool const operator<=(const T y) const { return repr() <= y.repr(); }
  constexpr bool const operator>=(const T y) const { return repr() >= y.repr(); }
  constexpr bool const operator==(const T y) const { return repr() == y.repr(); }
  constexpr bool const operator!=(const T y) const { return repr() != y.repr(); }

  constexpr T min(const T y) const { return *this < y ? *this : y; }
  constexpr T max(const T y) const { return *this < y ? y : *this; }
  constexpr T clip(const T x, const T y) const { return max(x).min(y); }

  // saturates between -1 and 1 (signed) or 0 and 1 (unsigned)
  constexpr T clip() const {
    return SIGN ? saturate<FRAC+1>() : saturate<FRAC>();
  }

  // saturating add/sub
  constexpr T const add_sat(const T y) const {
#ifdef __arm__
    static_assert(!(WIDTH==32 && SIGN==UNSIGNED), "Unsigned saturating add unsupported");
    if (WIDTH == 32) {
      if (SIGN) return T(DANGER, __QADD(val_, y.val_));
      else return T(DANGER, 42); // unreachable: there is no UQADD instruction
    } else if (WIDTH == 16) {
      if (SIGN) return T(DANGER, __QADD16(val_, y.val_));
      else return T(DANGER, __UQADD16(val_, y.val_));
    } else if (WIDTH == 8) {
      if (SIGN) return T(DANGER, __QADD8(val_, y.val_));
      else return T(DANGER, __UQADD8(val_, y.val_));
    }
#else
    using Wider = typename Basetype<WIDTH, SIGN>::Wider;
    Wider r = (Wider)val_ + (Wider)y.val_;
    r = saturate_integer<Wider, WIDTH>(r);
    return T(DANGER, r);
#endif
  }

  constexpr T sub_sat(const T y) const {
#ifdef __arm__
    static_assert(!(WIDTH==32 && SIGN==UNSIGNED), "Unsigned saturating add unsupported");
    if (WIDTH == 32) {
      if (SIGN) return T(DANGER, __QSUB(val_, y.val_));
      else return T(DANGER, 42); // unreachable: there is no UQADD instruction
    } else if (WIDTH == 16) {
      if (SIGN) return T(DANGER, __QSUB16(val_, y.val_));
      else return T(DANGER, __UQSUB16(val_, y.val_));
    } else if (WIDTH == 8) {
      if (SIGN) return T(DANGER, __QSUB8(val_, y.val_));
      else return T(DANGER, __UQSUB8(val_, y.val_));
    }
#else
    using Wider = typename Basetype<WIDTH, SIGN>::Wider;
    Wider r = (Wider)val_ - (Wider)y.val_;
    r = saturate_integer<Wider, WIDTH>(r);
    return T(DANGER, r);
#endif
  }

};

using s16_0 = Fixed<SIGNED, 16, 0>;
using u16_0 = Fixed<UNSIGNED, 16, 0>;
using s32_0 = Fixed<SIGNED, 32, 0>;
using u32_0 = Fixed<UNSIGNED, 32, 0>;

using s16 = s16_0;
using u16 = u16_0;
using s32 = s32_0;
using u32 = u32_0;

using s = s16;
using u = u16;

using s1_15 = Fixed<SIGNED, 1, 15>;
using s17_15 = Fixed<SIGNED, 17, 15>;
using u0_16 = Fixed<UNSIGNED, 0, 16>;
using s1_31 = Fixed<SIGNED, 1, 31>;
using u0_32 = Fixed<UNSIGNED, 0, 32>;
using s10_22 = Fixed<SIGNED, 10, 22>;
using u10_22 = Fixed<UNSIGNED, 10, 22>;

// Some user-defined literals

constexpr s16 operator "" _s(const unsigned long long int x) { return s16::of_int(x); }
constexpr u16 operator "" _u(unsigned long long int x) { return u16::of_int(x); }
constexpr s16 operator "" _s16(const unsigned long long int x) { return s16::of_int(x); }
constexpr u16 operator "" _u16(unsigned long long int x) { return u16::of_int(x); }
constexpr s32 operator "" _s32(unsigned long long int x) { return s32::of_int(x); }
constexpr u32 operator "" _u32(unsigned long long int x) { return u32::of_int(x); }
constexpr s1_15 operator "" _s1_15(long double x) { return s1_15(x); }
constexpr s17_15 operator "" _s17_15(long double x) { return s17_15(x); }
constexpr u0_16 operator "" _u0_16(long double x) { return u0_16(x); }
constexpr s1_31 operator "" _s1_31(long double x) { return s1_31(x); }
constexpr u0_32 operator "" _u0_32(long double x) { return u0_32(x); }
constexpr u10_22 operator "" _u10_22(long double x) { return u10_22(x); }
constexpr s10_22 operator "" _s10_22(long double x) { return s10_22(x); }
