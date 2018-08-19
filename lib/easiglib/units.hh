#include "numtypes.hh"
#include "data.hh"

struct Pitch : private Float {
  using T = Pitch;

  explicit constexpr Pitch(Float x) : Float(x) {};
  constexpr Float const repr() const { return *this; }

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
};

constexpr Pitch operator "" _st(long double x) { return Pitch(f(x)); }

struct Freq : private Float {

  static constexpr f semitones_to_ratio(f p) {
    return Math::fast_exp2(p/12_f);
  }

  explicit constexpr Freq(Float x) : Float(x / f(kSampleRate)) {};
  explicit constexpr Freq(Pitch p) : Freq(semitones_to_ratio(p.repr() - 69._f) * 440_f) { };

  constexpr Float const repr() const { return *this; }

};

constexpr Freq operator "" _Hz(long double x) { return Freq(f(x)); }
