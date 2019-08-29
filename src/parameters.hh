#pragma once

#include "numtypes.hh"

constexpr struct Frame {
  s1_15 l = 0._s1_15;
  s1_15 r = 0._s1_15;
} zero;

constexpr int kUiUpdateRate = 200; // Hz
constexpr int kSampleRate = 48000; // Hz
constexpr int kBlockSize = 8;
constexpr int kMaxNumOsc = 16;

enum TwistMode { FEEDBACK, PULSAR, CRUSH };
enum WarpMode { FOLD, CHEBY, SEGMENT };
enum GridMode { TWELVE, OCTAVE, FREE };
enum ModulationMode { ONE, TWO, THREE };

enum SplitMode { ALTERNATE, LOW_HIGH, LOWEST_REST };

struct Parameters {
  f tilt;                        // -1..1
  f root;                        // semitones
  f pitch;                       // midi note
  f spread;                      // semitones
  f detune;                      // semitones

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

  // Alt. parameters

  int numOsc = kMaxNumOsc;      // 0..kMaxNumOsc
  SplitMode stereo_mode = ALTERNATE;
  SplitMode freeze_mode = LOW_HIGH;
  f crossfade_factor = 0.125_f;

  f new_note, fine_tune;
};

enum EventType {
  ButtonPush,
  ButtonRelease,
  ButtonTimeout,
  GateOn,
  GateOff,
  SwitchGrid,
  SwitchMod,
  SwitchTwist,
  SwitchWarp,
  PotMove,
  NewNote,
  GridChange,
  AltParamChange,
  StartCatchup,
  EndOfCatchup,
};

struct Event {
  EventType type;
  int data;
};
