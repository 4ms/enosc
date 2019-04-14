#pragma once

#include <cstdint>
#include <cmath>
#include <cfloat>

#include "util.hh"

#ifdef __arm__
  #include "stm32f7xx.h"
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

  // TODO: constructor from Fixed types

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

  constexpr T const abs() const { return T(val_ > 0 ? val_ : -val_); }
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

  T min(const T y) const {
#ifdef __arm__
    float res;
    __ASM volatile ("VMINNM.F32 %0, %1, %2" : "=t" (res) : "t" (val_), "t" (y.repr()));
    return T(res);
#else
    return *this < y ? *this : y;
#endif
  }

  T max(const T y) const {
#ifdef __arm__
    float res;
    __ASM volatile ("VMAXNM.F32 %0, %1, %2" : "=t" (res) : "t" (val_), "t" (y.repr()));
    return T(res);
#else
    return *this < y ? y : *this;
#endif
  }

  constexpr T clip(const T x, const T y) const { return max(x).min(y); }
  constexpr T clip() const { return clip(T(-1.0f), T(1.0f)); }

  constexpr int32_t floor() const {
    return static_cast<int32_t>(val_);
  }

  T integral() const {
#ifdef __arm__
    float res;
    __ASM volatile ("VRINTZ.F32 %0, %1" : "=t" (res) : "t" (val_));
    return T(res);
#else
    return T(static_cast<float>(floor()));
#endif
  }

  constexpr T fractional() const {
    return *this - integral();
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
private:
  __fp16 val_;
};

using f16 = Float16;

#endif

/***************
 * Fixed-Point
 ***************/

enum class sign {
  UNSIGNED,
  SIGNED
};

constexpr sign SIGNED = sign::SIGNED;
constexpr sign UNSIGNED = sign::UNSIGNED;

template<int WIDTH, sign SIGN> struct Basetype;

template<> struct Basetype<8, SIGNED> { using T = int8_t; };
template<> struct Basetype<8, UNSIGNED> { using T = uint8_t; };
template<> struct Basetype<16, SIGNED> { using T = int16_t; };
template<> struct Basetype<16, UNSIGNED> { using T = uint16_t; };
template<> struct Basetype<32, SIGNED> { using T = int32_t; };
template<> struct Basetype<32, UNSIGNED> { using T = uint32_t; };
#ifndef __arm__
template<> struct Basetype<64, SIGNED> { using T = int64_t; };
template<> struct Basetype<64, UNSIGNED> { using T = uint64_t; };
#endif

template<sign SIGN, int INT, int FRAC>
class Fixed {
  static constexpr const int WIDTH = INT + FRAC;

  using Base = typename Basetype<WIDTH, SIGN>::T;
  using T = Fixed<SIGN, INT, FRAC>;

  Base val_;

  template <typename A, int BITS>
  static constexpr A const saturate_integer(A x) {
    if (SIGN==SIGNED) {
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
    if (SIGN==SIGNED) return T::of_repr(__SSAT(val_, BITS));
    else return T::of_repr(__USAT(val_, BITS));
  }
#else
  template <int BITS>
  constexpr T const saturate() const {
    static_assert(BITS > 0 && BITS < WIDTH, "Invalid bit count");
    return T::of_repr(saturate_integer<Base, BITS>(val_));
  }
#endif
  enum class dangerous { DANGER };
  explicit constexpr Fixed(dangerous, Base x) : val_(x) {}

public:

  // CONSTRUCTORS

  // default constructor for arrays
  explicit Fixed() {}

  // unsafe constructor from representation
  static constexpr T of_repr(Base x) {
    return T(dangerous::DANGER, x);
  }

  // from base types, for literal notations (see below)
  static constexpr T of_long_long(unsigned long long int x) {
    return T::of_repr(x * (1ULL << FRAC));
  }

  static constexpr T of_double(long double x) {
    return T(dangerous::DANGER, static_cast<long long int>((x * (long double)(1ULL << FRAC))));
  }

  // from Float:
  explicit constexpr Fixed(Float x) :
    val_(static_cast<Base>((x * Float(1ULL << FRAC)).repr())) { }

  static constexpr T inclusive(Float x) {
    return T::of_repr(static_cast<Base>((x * Float((1ULL << FRAC) - 1)).repr()));
  }

  // from Fixed:
  template<int INT2, int FRAC2>
  explicit constexpr Fixed(Fixed<SIGN, INT2, FRAC2> const that) :
    val_(Fixed<SIGN, INT, FRAC>::of_repr((unsigned)that.repr() << (FRAC - FRAC2)).repr()) {
    static_assert(FRAC2 <= FRAC, "Conversion with possible loss of precision");
    static_assert(INT2 <= INT, "Conversion with possible wrapover");
  }

  // from Fixed with truncation of fractional part (allows loss of precision)
  template<int INT2, int FRAC2>
  static constexpr T narrow(Fixed<SIGN, INT2, FRAC2> const that) {
    static_assert(FRAC2 > FRAC, "This is not a narrowing: use default constructor");
    static_assert(INT2 <= INT, "Conversion with possible wrapover");
    return T::of_repr(that.repr() >> (FRAC2 - FRAC));
  }

  // from Fixed with truncation of integer & fractional part (allows
  // wrapping & loss of precision); also allows changing sign!
  // TODO test
  template<sign SIGN2, int INT2, int FRAC2>
  static constexpr T wrap(Fixed<SIGN2, INT2, FRAC2> const that) {
    static_assert(INT2 > INT, "This does not wrap: use default constructor");
    if (FRAC2 >= FRAC) {
      return T::of_repr(that.repr() >> (FRAC2 - FRAC));
    } else
      return T::of_repr(that.repr() << (FRAC - FRAC2));
  }

  // Boundary values:
  static constexpr T min_val = T::of_repr(SIGN==SIGNED ? (Base)(1 << (WIDTH-1)) : 0);
  static constexpr T max_val = T::of_repr(SIGN==SIGNED ? (Base)(~(1 << (WIDTH-1))) : -1);
  static constexpr T increment = T::of_repr(1);

  // CONVERSIONS

  // unsafe getter for representation
  constexpr Base repr() const { return val_; }

  // TODO put these as constructors in Float
  constexpr Float const to_float() const { return Float(repr()) / Float(1ULL << FRAC); }
  constexpr Float const to_float_inclusive() const { return Float(repr()) / Float((1ULL << FRAC) - 1); }

  // TODO optimize for ARM: SAT/USAT instructions have built-in shift
  template <int INT2, int FRAC2>
  constexpr Fixed<SIGN, INT2, FRAC2> const to_sat() const {
    static_assert(INT2 < INT, "this is for saturation, use to()");
    Base x = saturate<WIDTH-(INT-INT2)>().repr();

    if (FRAC2 >= FRAC) {
      return Fixed<SIGN, INT2, FRAC2>::of_repr((unsigned)x << (FRAC2 - FRAC));
    } else {
      return Fixed<SIGN, INT2, FRAC2>::of_repr(x >> (FRAC - FRAC2));
    }
  }

  // preserves the represented value
  constexpr Fixed<SIGNED, INT+1, FRAC-1> const to_signed() const {
    static_assert(SIGN==UNSIGNED, "Only signed-to-unsigned conversion supported");
    return Fixed<SIGNED, INT+1, FRAC-1>::of_repr((signed)(val_ >> 1));
  }

  // remaps MIN..MAX to MIN..MAX (e.g. 0..1 --> -1..1)
  constexpr Fixed<SIGNED, INT+1, FRAC-1> const to_signed_scale() const {
    static_assert(SIGN==UNSIGNED, "Only signed-to-unsigned conversion supported");
    return Fixed<SIGNED, INT+1, FRAC-1>::of_repr((signed)(val_ - (1 << (WIDTH-1))));
  }

  // preserves the represented value if it is positive
  constexpr Fixed<UNSIGNED, INT-1, FRAC+1> const to_unsigned() const {
    static_assert(SIGN==SIGNED, "Only unsigned-to-signed conversion supported");
    return Fixed<UNSIGNED, INT-1, FRAC+1>::of_repr((unsigned)(val_ << 1));
  }

  // remaps MIN..MAX to MIN..MAX (e.g. -1..1 --> 0..1)
  constexpr Fixed<UNSIGNED, INT-1, FRAC+1> const to_unsigned_scale() const {
    static_assert(SIGN==SIGNED, "Only unsigned-to-signed conversion supported");
    return Fixed<UNSIGNED, INT-1, FRAC+1>::of_repr((unsigned)(val_ + (1 << (WIDTH-1))));
  }

  template <int SHIFT>
  constexpr Fixed<SIGN, INT+SHIFT, FRAC-SHIFT> const movr() const {
    return Fixed<SIGN, INT+SHIFT, FRAC-SHIFT>::of_repr(val_);
  }

  template <int SHIFT>
  constexpr Fixed<SIGN, INT-SHIFT, FRAC+SHIFT> const movl() const {
    return Fixed<SIGN, INT-SHIFT, FRAC+SHIFT>::of_repr(val_);
  }

  template <int SHIFT>
  constexpr Fixed<SIGN, INT+SHIFT, FRAC-SHIFT> const shiftr() const {
    return Fixed<SIGN, INT+SHIFT, FRAC-SHIFT>::of_repr(val_ >> SHIFT);
  }

  template <int SHIFT>
  constexpr Fixed<SIGN, INT-SHIFT, FRAC+SHIFT> const shiftl() const {
    return Fixed<SIGN, INT-SHIFT, FRAC+SHIFT>::of_repr(val_ << SHIFT);
  }

  // Operations:

  // in/decrement by the smallest amount possible in the representation
  constexpr T const succ() const { return T::of_repr(repr()+1L); }
  constexpr T const pred() const { return T::of_repr(repr()-1L); }

  constexpr T const floor() const { return T::of_repr(repr() & ~((1ULL << FRAC) - 1ULL)); }
  constexpr T const frac() const { return T::of_repr(repr() & ((1ULL << FRAC) - 1ULL)); }
  constexpr T const abs() const { return T::of_repr(libc_abs(val_)); }

  constexpr Fixed<SIGN, WIDTH, 0> const integral() const {
    return Fixed<SIGN, WIDTH, 0>::narrow(*this);
  }

  constexpr Fixed<SIGN, 0, WIDTH> const fractional() const {
    return Fixed<SIGN, 0, WIDTH>::wrap(*this);
  }

  constexpr T const operator-() const {
    static_assert(SIGN==SIGNED, "Prefix negation is invalid on unsigned data");
    return T::of_repr(-repr());
  }

  template<int INT2, int FRAC2>
  constexpr auto const operator+(Fixed<SIGN, INT2, FRAC2> const y) const {
    using T = Fixed<SIGN, Max<int, INT, INT2>::val, Max<int, FRAC, FRAC2>::val>;
    T a = T(*this);
    T b = T(y);
    return T::of_repr(a.repr() + b.repr());
  }

  template<int INT2, int FRAC2>
  constexpr auto const operator-(Fixed<SIGN, INT2, FRAC2> const y) const {
    using T = Fixed<SIGN, Max<int, INT, INT2>::val, Max<int, FRAC, FRAC2>::val>;
    T a = T(*this);
    T b = T(y);
    return T::of_repr(a.repr() - b.repr());
  }

  template<int INT2, int FRAC2>
  constexpr auto
  operator*(Fixed<SIGNED, INT2, FRAC2> const that) const {
    using T = Fixed<SIGNED, INT+INT2-1, FRAC+FRAC2+1>;
    // TODO understand this * 2!
    return T::of_repr((repr() * that.repr()) * 2);
  }

  template<int INT2, int FRAC2>
  constexpr auto
  operator*(Fixed<UNSIGNED, INT2, FRAC2> const that) const {
    using T = Fixed<UNSIGNED, INT+INT2, FRAC+FRAC2>;
    return T::of_repr((repr() * that.repr()));
  }

  template<int INT2, int FRAC2>
  constexpr auto
  operator/(Fixed<SIGN, INT2, FRAC2> const that) const {
    using Wider = typename Basetype<WIDTH+INT2+FRAC2, SIGN>::T;
    Wider x = repr();
    x <<= INT2 + FRAC2;
    return Fixed<SIGN, INT+FRAC2, FRAC+INT2>::of_repr(x / that.repr());
  }

  // Degraded basic operators for
  constexpr T operator*(Base const y) const {
    return T::of_repr(repr() * y);
  }

  constexpr T operator/(Base const y) const {
    return T::of_repr(repr() / y);
  }

  template <int BITS>
  constexpr T div2() const {
    static_assert(BITS >= 0 && BITS < WIDTH, "Invalid bit count");
    return this->template shiftr<BITS>().template movl<BITS>();
  }

  constexpr void operator+=(T const that) { *this = *this + that; }
  constexpr void operator-=(T const that) { *this = *this - that; }
  constexpr void operator*=(T const that) { *this = T::narrow(*this * that); }
  constexpr void operator/=(T const that) { *this = T::narrow(*this / that); }
  constexpr void operator*=(Base const that) { val_ = (*this * that).repr(); }
  constexpr void operator/=(Base const that) { val_ = (*this / that).repr(); }

  constexpr T operator++() { T temp = *this; *this += T(1_f); return temp; }
  constexpr T operator--() { T temp = *this; *this -= T(1_f); return temp; }
  constexpr T operator++(int) { *this += T(1_f); return *this; }
  constexpr T operator--(int) { *this -= T(1_f); return *this; }

  constexpr T operator%(T const that) const {
    static_assert(SIGN==UNSIGNED, "modulo undefined on unsigned types");
    return T::of_repr(val_ % that.val_);
  }

  template<int INT2, int FRAC2>
  constexpr bool const operator<(Fixed<SIGN, INT2, FRAC2> const that) const {
    using T = Fixed<SIGN, Max<int, INT, INT2>::val, Max<int, FRAC, FRAC2>::val>;
    return T(*this).repr() < T(that).repr();
  }

  template<int INT2, int FRAC2>
  constexpr bool const operator>(Fixed<SIGN, INT2, FRAC2> const that) const {
    using T = Fixed<SIGN, Max<int, INT, INT2>::val, Max<int, FRAC, FRAC2>::val>;
    return T(*this).repr() > T(that).repr();
  }

  template<int INT2, int FRAC2>
  constexpr bool const operator<=(Fixed<SIGN, INT2, FRAC2> const that) const {
    using T = Fixed<SIGN, Max<int, INT, INT2>::val, Max<int, FRAC, FRAC2>::val>;
    return T(*this).repr() <= T(that).repr();
  }

  template<int INT2, int FRAC2>
  constexpr bool const operator>=(Fixed<SIGN, INT2, FRAC2> const that) const {
    using T = Fixed<SIGN, Max<int, INT, INT2>::val, Max<int, FRAC, FRAC2>::val>;
    return T(*this).repr() >= T(that).repr();
  }

  template<int INT2, int FRAC2>
  constexpr bool const operator==(Fixed<SIGN, INT2, FRAC2> const that) const {
    using T = Fixed<SIGN, Max<int, INT, INT2>::val, Max<int, FRAC, FRAC2>::val>;
    return T(*this).repr() == T(that).repr();
  }

  template<int INT2, int FRAC2>
  constexpr bool const operator!=(Fixed<SIGN, INT2, FRAC2> const that) const {
    using T = Fixed<SIGN, Max<int, INT, INT2>::val, Max<int, FRAC, FRAC2>::val>;
    return T(*this).repr() != T(that).repr();
  }

  template<int INT2, int FRAC2>
  constexpr auto min(const Fixed<SIGN, INT2, FRAC2> that) const {
    return *this < that ? *this : that;
  }

  template<int INT2, int FRAC2>
  constexpr auto max(const Fixed<SIGN, INT2, FRAC2> that) const {
    return *this < that ? that : *this;
  }

  constexpr T clip(const T x, const T y) const { return max(x).min(y); }

  // saturates between -1 and 1 (signed) or 0 and 1 (unsigned)
  constexpr T clip() const {
    return SIGN==SIGNED ? saturate<FRAC+1>() : saturate<FRAC>();
  }

  // saturating add/sub
  constexpr T const add_sat(const T y) const {
#ifdef __arm__
    static_assert(!(WIDTH==32 && SIGN==UNSIGNED), "Unsigned saturating add unsupported");
    if (WIDTH == 32) {
      if (SIGN==SIGNED) return T::of_repr(__QADD(val_, y.val_));
      else return T::of_repr(42); // unreachable: there is no UQADD instruction
    } else if (WIDTH == 16) {
      if (SIGN==SIGNED) return T::of_repr(__QADD16(val_, y.val_));
      else return T::of_repr(__UQADD16(val_, y.val_));
    } else if (WIDTH == 8) {
      if (SIGN==SIGNED) return T::of_repr(__QADD8(val_, y.val_));
      else return T::of_repr(__UQADD8(val_, y.val_));
    }
#else
    using Wider = typename Basetype<WIDTH*2, SIGN>::T;
    Wider r = (Wider)val_ + (Wider)y.val_;
    r = saturate_integer<Wider, WIDTH>(r);
    return T::of_repr(r);
#endif
  }

  constexpr T sub_sat(const T y) const {
#ifdef __arm__
    static_assert(!(WIDTH==32 && SIGN==UNSIGNED), "Unsigned saturating add unsupported");
    if (WIDTH == 32) {
      if (SIGN==SIGNED) return T::of_repr(__QSUB(val_, y.val_));
      else return T::of_repr(42); // unreachable: there is no UQADD instruction
    } else if (WIDTH == 16) {
      if (SIGN==SIGNED) return T::of_repr(__QSUB16(val_, y.val_));
      else return T::of_repr(__UQSUB16(val_, y.val_));
    } else if (WIDTH == 8) {
      if (SIGN==SIGNED) return T::of_repr(__QSUB8(val_, y.val_));
      else return T::of_repr(__UQSUB8(val_, y.val_));
    }
#else
    using Wider = typename Basetype<WIDTH*2, SIGN>::T;
    Wider r = (Wider)val_ - (Wider)y.val_;
    r = saturate_integer<Wider, WIDTH>(r);
    return T::of_repr(r);
#endif
  }

};

using u0_16 = Fixed<UNSIGNED, 0, 16>;
using u1_15 = Fixed<UNSIGNED, 1, 15>;
using u2_14 = Fixed<UNSIGNED, 2, 14>;
using u3_13 = Fixed<UNSIGNED, 3, 13>;
using u4_12 = Fixed<UNSIGNED, 4, 12>;
using u5_11 = Fixed<UNSIGNED, 5, 11>;
using u6_10 = Fixed<UNSIGNED, 6, 10>;
using u7_9 = Fixed<UNSIGNED, 7, 9>;
using u8_8 = Fixed<UNSIGNED, 8, 8>;
using u9_7 = Fixed<UNSIGNED, 9, 7>;
using u10_6 = Fixed<UNSIGNED, 10, 6>;
using u11_5 = Fixed<UNSIGNED, 11, 5>;
using u12_4 = Fixed<UNSIGNED, 12, 4>;
using u13_3 = Fixed<UNSIGNED, 13, 3>;
using u14_2 = Fixed<UNSIGNED, 14, 2>;
using u15_1 = Fixed<UNSIGNED, 15, 1>;
using u16_0 = Fixed<UNSIGNED, 16, 0>;

using s0_16 = Fixed<SIGNED, 0, 16>;
using s1_15 = Fixed<SIGNED, 1, 15>;
using s2_14 = Fixed<SIGNED, 2, 14>;
using s3_13 = Fixed<SIGNED, 3, 13>;
using s4_12 = Fixed<SIGNED, 4, 12>;
using s5_11 = Fixed<SIGNED, 5, 11>;
using s6_10 = Fixed<SIGNED, 6, 10>;
using s7_9 = Fixed<SIGNED, 7, 9>;
using s8_8 = Fixed<SIGNED, 8, 8>;
using s9_7 = Fixed<SIGNED, 9, 7>;
using s10_6 = Fixed<SIGNED, 10, 6>;
using s11_5 = Fixed<SIGNED, 11, 5>;
using s12_4 = Fixed<SIGNED, 12, 4>;
using s13_3 = Fixed<SIGNED, 13, 3>;
using s14_2 = Fixed<SIGNED, 14, 2>;
using s15_1 = Fixed<SIGNED, 15, 1>;
using s16_0 = Fixed<SIGNED, 16, 0>;

using u16_16 = Fixed<UNSIGNED, 16, 16>;
using s16_16 = Fixed<SIGNED, 16, 16>;
using s17_15 = Fixed<SIGNED, 17, 15>;
using s15_17 = Fixed<SIGNED, 15, 17>;
using s1_31 = Fixed<SIGNED, 1, 31>;
using u0_32 = Fixed<UNSIGNED, 0, 32>;
using u8_24 = Fixed<UNSIGNED, 8, 24>;
using u24_8 = Fixed<UNSIGNED, 24, 8>;
using s10_22 = Fixed<SIGNED, 10, 22>;
using u10_22 = Fixed<UNSIGNED, 10, 22>;

using s16_0 = Fixed<SIGNED, 16, 0>;
using u16_0 = Fixed<UNSIGNED, 16, 0>;
using s32_0 = Fixed<SIGNED, 32, 0>;
using u32_0 = Fixed<UNSIGNED, 32, 0>;

using s8_0 = Fixed<SIGNED, 8, 0>;
using s7_1 = Fixed<SIGNED, 7, 1>;
using s6_2 = Fixed<SIGNED, 6, 2>;
using s5_3 = Fixed<SIGNED, 5, 3>;
using s4_4 = Fixed<SIGNED, 4, 4>;
using s3_5 = Fixed<SIGNED, 3, 5>;
using s2_6 = Fixed<SIGNED, 2, 6>;
using s1_7 = Fixed<SIGNED, 1, 7>;
using s0_8 = Fixed<SIGNED, 0, 8>;

using u8_0 = Fixed<UNSIGNED, 8, 0>;
using u7_1 = Fixed<UNSIGNED, 7, 1>;
using u6_2 = Fixed<UNSIGNED, 6, 2>;
using u5_3 = Fixed<UNSIGNED, 5, 3>;
using u4_4 = Fixed<UNSIGNED, 4, 4>;
using u3_5 = Fixed<UNSIGNED, 3, 5>;
using u2_6 = Fixed<UNSIGNED, 2, 6>;
using u1_7 = Fixed<UNSIGNED, 1, 7>;
using u0_8 = Fixed<UNSIGNED, 0, 8>;

using s8 = s8_0;
using u8 = u8_0;

using s16 = s16_0;
using u16 = u16_0;

using s32 = s32_0;
using u32 = u32_0;

using s = s16;
using u = u16;

// Some user-defined literals

constexpr s16 operator "" _s(const unsigned long long int x) { return s16::of_long_long(x); }
constexpr u16 operator "" _u(unsigned long long int x) { return u16::of_long_long(x); }
constexpr s16 operator "" _s16(const unsigned long long int x) { return s16::of_long_long(x); }
constexpr u16 operator "" _u16(unsigned long long int x) { return u16::of_long_long(x); }
constexpr s32 operator "" _s32(unsigned long long int x) { return s32::of_long_long(x); }
constexpr u32 operator "" _u32(unsigned long long int x) { return u32::of_long_long(x); }
constexpr s1_15 operator "" _s1_15(long double x) { return s1_15::of_double(x); }
constexpr s16_16 operator "" _s16_16(long double x) { return s16_16::of_double(x); }
constexpr s17_15 operator "" _s17_15(long double x) { return s17_15::of_double(x); }
constexpr u0_16 operator "" _u0_16(long double x) { return u0_16::of_double(x); }
constexpr s1_31 operator "" _s1_31(long double x) { return s1_31::of_double(x); }
constexpr u0_32 operator "" _u0_32(long double x) { return u0_32::of_double(x); }
constexpr u10_22 operator "" _u10_22(long double x) { return u10_22::of_double(x); }
constexpr s10_22 operator "" _s10_22(long double x) { return s10_22::of_double(x); }
constexpr u0_8 operator "" _u0_8(long double x) { return u0_8::of_double(x); }
constexpr s1_7 operator "" _s1_7(long double x) { return s1_7::of_double(x); }
constexpr s8_8 operator "" _s8_8(long double x) { return s8_8::of_double(x); }

template<sign SIGN, int INT, int FRAC>
constexpr Fixed<SIGN, INT, FRAC> Fixed<SIGN, INT, FRAC>::min_val;
template<sign SIGN, int INT, int FRAC>
constexpr Fixed<SIGN, INT, FRAC> Fixed<SIGN, INT, FRAC>::max_val;
template<sign SIGN, int INT, int FRAC>
constexpr Fixed<SIGN, INT, FRAC> Fixed<SIGN, INT, FRAC>::increment;
