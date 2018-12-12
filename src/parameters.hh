#include "numtypes.hh"

#pragma once

constexpr struct Frame {
  s1_15 l = 0._s1_15;
  s1_15 r = 0._s1_15;
} zero;

constexpr int kBlockSize = 16;
constexpr int kSampleRate = 96000;
constexpr int kNumOsc = 16;

enum TwistMode { FEEDBACK, PULSAR, DECIMATE };
enum WarpMode { FOLD, CHEBY, CRUSH };
enum StereoMode { ALTERNATE, SPLIT, LOWER_REST };

// textile oscillator:
enum DivisionMode { INTEGER, ODD, POW_OF_TWO };
enum TranspositionMode { CHROMATIC, THREE_ST, OCTAVE };

struct Parameters {
  f pitch;                       // midi note
  f spread;                      // semitones
  f detune;                     // semitones

  struct Twist {
    TwistMode mode;
    f value;                    // 0..1
  } twist;

  struct Warp {
    WarpMode mode;
    f value;                    // 0..1
  } warp;

  StereoMode stereo_mode = ALTERNATE;

  // textile oscillator:
  f pitch_offset = 0.2_f;
  f timbre = 0.1_f;
  f delay = 0_f;
  f drunk_delay = 0_f;
  f polyphony = 1_f;
  f ornament_proba = 0_f;
  f proba_window_scale = 0_f;
  f division = 0_f;
  int division_window = 16;

  f transposition = 0_f;

  f attack = 0.0002_f;
  f decay = 0.00002_f;

  DivisionMode division_mode;
  TranspositionMode transposition_mode;

  bool quantize;
};
