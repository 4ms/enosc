#include "numtypes.hh"
#include "data.hh"

struct Pitch : Float {
};

struct Freq : private Float {

  explicit constexpr Freq(Float x) : Float(x / (Float)kSampleRate) { };

  constexpr Float const repr() const { return *this; }

  static Float semitones_to_ratio(Float semitones) { // -127..128
    semitones += 128_f;
    index integral = index(semitones); // 0..255
    Float fractional = semitones.fractional();
    index low_index = index((fractional * Float(Data::pitch_ratios_low.size().repr())));
    return
      Data::pitch_ratios_high[integral] *
      Data::pitch_ratios_low[low_index];
  }

  static Float of_midi(Float midi_pitch) {
    return semitones_to_ratio(midi_pitch - 69_f) * 440_f;
  }
};

constexpr Freq operator "" _Hz(long double x) { return Freq(f(x)); }
