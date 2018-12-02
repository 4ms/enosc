#include "parameters.hh"        // TODO bad
#include "numtypes.hh"
#include "data.hh"
#include "math.hh"

struct Freq : private Float {

  static constexpr f semitones_to_ratio(f p) {
    return Math::fast_exp2(p/12_f);
  }

  explicit constexpr Freq(f x) : Float(x / f(kSampleRate)) {};
  static constexpr Freq of_pitch(f p) {
    return Freq(semitones_to_ratio(p - 69._f) * 440_f);
  }

  constexpr Float const repr() const { return *this; }
  constexpr u0_32 to_increment() const { return u0_32(this->repr()); }

};

constexpr Freq operator "" _Hz(long double x) { return Freq(f(x)); }
