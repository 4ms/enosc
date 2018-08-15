#include "numtypes.hh"
#include "data.hh"

struct Freq {
  static Float Hz(Float x) { return x / Float(kSampleRate); }

  static Float semitones_to_ratio(Float semitones) { // -127..128
    semitones += 128_f;
    int integral = s(semitones).repr(); // 0..255
    Float fractional = semitones - Float(integral);    // 0..1
    int low_index = s(fractional * Float(Data::pitch_ratios_low.size().repr())).repr();
    return
      // TODO: better solution for data lookup ~> Float
      Float(Data::pitch_ratios_high[index(integral)]) *
      Float(Data::pitch_ratios_low[index(low_index)]);
  }

  static Float of_midi(Float midi_pitch) {
    return semitones_to_ratio(midi_pitch - 69_f) * 440_f;
  }
};
