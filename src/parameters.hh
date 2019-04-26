#include "numtypes.hh"

#pragma once

constexpr struct Frame {
  s1_15 l = 0._s1_15;
  s1_15 r = 0._s1_15;
} zero;

constexpr int kBlockSize = 16;
constexpr int kSampleRate = 96000;
constexpr int kMaxNumOsc = 10;

enum TwistMode { FEEDBACK, PULSAR, DECIMATE };
enum WarpMode { FOLD, CHEBY, CRUSH };
enum GridMode { CHORD, HARM, JUST };
enum ModulationMode { ONE, TWO, THREE };

enum StereoMode { ALTERNATE, SPLIT, LOWER_REST };

// textile oscillator:
enum DivisionMode { INTEGER, ODD, POW_OF_TWO };
enum TranspositionMode { CHROMATIC, THREE_ST, OCTAVE };

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

  int selected_osc = 0;             // 0..kNumOsc
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
};

struct Event {
  EventType type;
  int data;
};
