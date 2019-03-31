#pragma once

#include "adc.hh"
#include "dsp.hh"
#include "polyptic_oscillator.hh"

const int kPotFiltering = 1;     // 0..16
const f kPotDeadZone = 0.01_f;
const f kPitchPotRange = 6_f * 12_f;
const f kRootPotRange = 10_f * 12_f;
const f kSpreadRange = 12_f;
const f kCalibration2Voltage = 4_f;
const f kChangeThreshold = 0.03_f;

enum Law { LINEAR, QUADRATIC, CUBIC, QUARTIC };

template<Law LAW>  // Lp = 0..16
class PotConditioner {
public:
  u0_16 Process(u0_16 x) {
    switch(LAW) {
    case LINEAR: break;
    case QUADRATIC: x = u0_16::narrow(x * x); break;
    case CUBIC: x = u0_16::narrow(u0_16::narrow(x * x) * x); break;
    case QUARTIC:
      x = u0_16::narrow(x * x);
      x = u0_16::narrow(x * x);
      break;
    }
    return x;
  }
};

struct CVConditioner {
  s1_15 Process(u0_16 in) {
    // TODO calibration
    s1_15 x = in.to_signed_scale();
    return x;                 // -1..1
  }
};

template<int size>
class AudioCVConditioner {
  Average<8, 1> lp_;
  CicDecimator<1, size> cic_;
  f offset;
  f slope;
public:
  AudioCVConditioner(f o, f s) : offset(o), slope(s) {}
  void calibrate_offset() {
    u0_16 o = lp_.last();
    offset = o.to_float_inclusive();
  }
  void calibrate_slope() {
    u0_16 reading = lp_.last();
    f octave = (reading.to_float_inclusive() - offset) / kCalibration2Voltage;
    slope = 12_f / octave;
  }
  f Process(Block<s1_15, size> in) {
    s1_15 x = in[0];
    cic_.Process(in.data(), &x, 1); // -1..1
    u0_16 y = x.to_unsigned_scale(); // 0..1
    y = lp_.Process(y);
    f z = y.to_float_inclusive(); // 0..1
    z -= offset;
    z *= slope;                // -24..72
    return z;
  }
};

struct None { s1_15 Process(u0_16) { return 0._s1_15; } };

template<class PotConditioner, class CVConditioner, int LP>
struct PotCVCombiner {
  PotConditioner pot_;
  CVConditioner cv_;
  QuadraticOnePoleLp<LP> lp_;
  f Process(u0_16 pot, u0_16 cv) {
    s17_15 x = s17_15(pot_.Process(pot).to_signed());
    // TODO use saturating add
    x -= s17_15(cv_.Process(cv));
    f y = x.to_float_inclusive().clip(0_f, 1.0_f);
    return lp_.Process(y);
  }
  f Process(u0_16 pot) {
    f x = pot_.Process(pot).to_float_inclusive();
    return lp_.Process(x);
  }
};

template<int size>
class Control {

  Adc adc_;

  PotCVCombiner<PotConditioner<LINEAR>, None, kPotFiltering> detune_;
  PotCVCombiner<PotConditioner<LINEAR>, CVConditioner, kPotFiltering> warp_;
  PotCVCombiner<PotConditioner<LINEAR>, CVConditioner, kPotFiltering> tilt_;
  PotCVCombiner<PotConditioner<LINEAR>, CVConditioner, kPotFiltering> twist_;
  PotCVCombiner<PotConditioner<LINEAR>, CVConditioner, kPotFiltering> grid_;
  PotCVCombiner<PotConditioner<LINEAR>, CVConditioner, kPotFiltering> mod_;
  PotCVCombiner<PotConditioner<LINEAR>, CVConditioner, kPotFiltering> spread_;

  PotConditioner<LINEAR> pitch_pot_;
  PotConditioner<LINEAR> root_pot_;
  AudioCVConditioner<size> pitch_cv_ {0.240466923_f, 96.8885345_f};
  AudioCVConditioner<size> root_cv_  {0.24319829_f, 97.4769897_f};
  QuadraticOnePoleLp<2> root_pot_lp_;
  QuadraticOnePoleLp<2> pitch_pot_lp_;

  // delay is 5 ms
  ChangeDetector<int(0.05 * kSampleRate / size)>
  pitch_cv_change_detector_ {0.005_f, 0.01_f};

  PolypticOscillator<size> &osc_;

  Sampler<f> pitch_cv_sampler_;

public:

  Control(PolypticOscillator<size> &osc) : osc_(osc) {}

  void Process(Block<Frame, size> codec_in, Parameters &params) {

    // Process codec input
    s1_15 in1[size], in2[size];
    Block<s1_15, size> pitch_block {in1};
    Block<s1_15, size> root_block {in2};

    for (auto x : zip(codec_in, pitch_block, root_block)) {
      Frame &in = get<0>(x);
      s1_15 &pi = get<1>(x);
      s1_15 &ro = get<2>(x);
      pi = in.l;
      ro = in.r;
    }

    // Process potentiometer & CV

    f detune = detune_.Process(adc_.detune_pot());
    detune = Math::crop_down(kPotDeadZone, detune);
    detune = (detune * detune) * (detune * detune);
    params.detune = detune;

    f tilt = tilt_.Process(adc_.tilt_pot(), adc_.tilt_cv());
    tilt = Math::crop(kPotDeadZone, tilt);
    tilt = tilt * 2_f - 1_f;
    tilt *= tilt * tilt;
    tilt *= 4_f;
    tilt = Math::fast_exp2(tilt);
    params.tilt = tilt;

    f warp = warp_.Process(adc_.warp_pot(), adc_.warp_cv());
    warp = Math::crop(kPotDeadZone, warp);
    params.warp.value = warp;

    f twist = twist_.Process(adc_.twist_pot(), adc_.twist_cv());
    twist = Math::crop(kPotDeadZone, twist);
    if (params.twist.mode == FEEDBACK) {
      twist *= twist * 0.7_f;
    } else if (params.twist.mode == PULSAR) {
      twist = 1_f - twist;
      twist *= twist;
    } else if (params.twist.mode == DECIMATE) {
      twist *= twist * 0.5_f;
    }
    params.twist.value = twist;

    f mod = mod_.Process(adc_.mod_pot(), adc_.mod_cv());
    mod = Math::crop(kPotDeadZone, mod);
    if (params.modulation.mode == ONE) {
      mod *= 0.9_f;
    } else if (params.modulation.mode == TWO) {
      mod *= 6.0_f;
    } else if (params.modulation.mode == THREE) {
      mod *= 4.0_f;
    }
    params.modulation.value = mod;

    f spread = spread_.Process(adc_.spread_pot(), adc_.spread_cv());
    spread = Math::crop(kPotDeadZone, spread);
    spread *= spread;
    params.spread = spread * kSpreadRange;

    f grid = grid_.Process(adc_.grid_pot(), adc_.grid_cv());
    grid = Math::crop(kPotDeadZone, grid); // [0..1]
    grid *= 9_f;                           // [0..9]
    grid += 0.5_f;                         // [0.5..9.5]
    params.grid.value = grid.floor(); // [0..9]

    // Root & Pitch
    u0_16 r = root_pot_.Process(adc_.root_pot());
    f root = root_pot_lp_.Process(r.to_float_inclusive());
    root *= kRootPotRange;
    root += root_cv_.Process(root_block);
    params.root = root;

    u0_16 p = pitch_pot_.Process(adc_.pitch_pot());
    f pitch = pitch_pot_lp_.Process(p.to_float_inclusive()); // 0..1
    pitch *= kPitchPotRange;                               // 0..range
    pitch -= kPitchPotRange * 0.5_f;                       // -range/2..range/2

    f pitch_cv = pitch_cv_.Process(pitch_block);
    bool pitch_cv_changed = pitch_cv_change_detector_.Process(pitch_cv);
    if (pitch_cv_changed) {
      osc_.new_note(pitch_cv);
    }

    pitch_cv = pitch_cv_sampler_.Process(pitch_cv);
    pitch += pitch_cv;
    params.pitch = pitch;

    // params.selected_osc = (r.to_float() * f(kNumOsc-1) + 0.5_f).floor();

    // Start next conversion
    adc_.Start();
  }

  void hold_pitch_cv() { pitch_cv_sampler_.hold(); }
  void release_pitch_cv() { pitch_cv_sampler_.release(); }

  void Calibrate1() {
    pitch_cv_.calibrate_offset();
    root_cv_.calibrate_offset();
  }
  void Calibrate2() {
    pitch_cv_.calibrate_slope();
    root_cv_.calibrate_slope();
  }
};
