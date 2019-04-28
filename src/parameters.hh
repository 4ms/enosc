#include "numtypes.hh"

#pragma once

constexpr struct Frame {
  s1_15 l = 0._s1_15;
  s1_15 r = 0._s1_15;
} zero;

constexpr int kUiUpdateRate = 100; // Hz
constexpr int kSampleRate = 96000; // Hz
constexpr int kBlockSize = 16;
constexpr int kProcessRate = kSampleRate / kBlockSize; // Hz
constexpr int kMaxNumOsc = 10;

enum TwistMode { FEEDBACK, PULSAR, DECIMATE };
enum WarpMode { FOLD, CHEBY, CRUSH };
enum GridMode { CHORD, HARM, JUST };
enum ModulationMode { ONE, TWO, THREE };

enum StereoMode { ALTERNATE, SPLIT, LOWER_REST };

struct Crossfade {
  static constexpr f linear = 0._f;
  static constexpr f mid = 0.2_f;
  static constexpr f steep = 0.4_f;
};

struct Parameters {
  int numOsc = kMaxNumOsc;
  f tilt;                       // -1..1
  f root;
  f pitch;                       // midi note
  f spread;                      // semitones
  f detune;                     // semitones

  struct Modulation {
    ModulationMode mode;
    f value;                 // 0..1
  } modulation;

  struct Grid {
    GridMode mode;
    int value;                     // 0..9
  } grid;

  struct Twist {
    TwistMode mode;
    f value;                    // 0..1
  } twist;

  struct Warp {
    WarpMode mode;
    f value;                    // 0..1
  } warp;

  StereoMode stereo_mode = ALTERNATE;
  f crossfade_factor = Crossfade::mid;
};

enum EventType {
  ButtonPush,
  ButtonRelease,
  ButtonTimeout,
  GateOn,
  GateOff,
  SwitchGridSwitched,
  SwitchModSwitched,
  SwitchTwistSwitched,
  SwitchWarpSwitched,
  PotMoved,
  NewNote,
  GridChanged,
  NumOscChanged,
  StartCatchup,
  EndOfCatchup,
};

struct Event {
  EventType type;
  int data;
};
