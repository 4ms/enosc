#pragma once

#include "adc.hh"
#include "dsp.hh"
#include "polyptic_oscillator.hh"
#include "event_handler.hh"

const f kPotDeadZone = 0.01_f;
const f kPitchPotRange = 6_f * 12_f;
const f kRootPotRange = 10_f * 12_f;
const f kNewNoteRange = 6_f * 12_f;
const f kNewNoteFineRange = 4_f;
const f kSpreadRange = 12_f;
const f kCalibration2Voltage = 4_f;
const f kCalibrationSuccessTolerance = 0.2_f;
const f kPotMoveThreshold = 0.01_f;

const int kCalibrationIterations = 16;

template<int block_size>
class AudioCVConditioner {
  Average<8, 1> lp_;
  CicDecimator<1, block_size> cic_;
  f offset_, nominal_offset_;
  f slope_, nominal_slope_;
  f last_;
  f last_raw_reading() { return f::inclusive(lp_.last()); }
public:
  AudioCVConditioner(f o, f s) :
    offset_(o), nominal_offset_(o),
    slope_(s), nominal_slope_(s) {}

  bool calibrate_offset() {
    f offset = last_raw_reading();
    if ((offset / nominal_offset_ - 1_f).abs() < kCalibrationSuccessTolerance) {
      offset_ = offset;
      return true;
    } else return false;
  }

  bool calibrate_slope() {
    f octave = (last_raw_reading() - offset_) / kCalibration2Voltage;
    f slope = 12_f / octave;

    if ((slope / nominal_slope_ - 1_f).abs() < kCalibrationSuccessTolerance) {
      slope_ = slope;
      return true;
    } else return false;
  }

  void Process(Buffer<s1_15, block_size>& in) {
    s1_15 x = in[0];
    cic_.Process(in.data(), &x, 1); // -1..1
    u0_16 y = x.to_unsigned_scale(); // 0..1
    y = lp_.Process(y);
    last_ = f::inclusive(y); // 0..1
    last_ -= offset_;
    last_ *= slope_;                // -24..72
  }

  f last() { return last_; }
};

template<AdcInput INPUT>
class CVConditioner {
  Adc& adc_;
  f offset_;

public:
  CVConditioner(Adc& adc, f offset) : adc_(adc), offset_(offset) {}

  bool calibrate_offset() {
    f reading = 0_f;

    for (int i=0; i<kCalibrationIterations; i++) {
      u0_16 in = adc_.get(INPUT);
      s1_15 x = in.to_signed_scale();
      reading += f::inclusive(x);
      HAL_Delay(1);
    }

    reading /= f(kCalibrationIterations);

    if (reading.abs() < 0.1_f) {
      offset_ = reading;
      return true;
    } else {
      return false;
    }
  }

  f Process() {
    u0_16 in = adc_.get(INPUT);
    s1_15 x = in.to_signed_scale();
    return f::inclusive(x) - offset_;
  }
};

struct NoCVInput {
  NoCVInput(Adc& adc, f offset) {}
  f Process() { return 0._f; }
};

class MovementDetector {
  f previous_value_;
public:
  bool Process(f x) {
    f diff = (x - previous_value_).abs();
    if (diff > kPotMoveThreshold) {
      previous_value_ = x;
      return true;
    } else return false;
  }
};

enum class Law { LINEAR, QUADRATIC, CUBIC, QUARTIC };

template<AdcInput INPUT, Law LAW, class FILTER>
class PotConditioner : MovementDetector {
  Adc& adc_;
  FILTER filter_;
public:
  PotConditioner(Adc& adc) : adc_(adc) {}

  f Process(std::function<void(Event)> const& put) {
    f x = f::inclusive(adc_.get(INPUT));
    x = Signal::crop(kPotDeadZone, x);
    switch(LAW) {
    case Law::LINEAR: break;
    case Law::QUADRATIC: x = x * x; break;
    case Law::CUBIC: x = x * x * x; break;
    case Law::QUARTIC: x = x * x; x = x * x; break;
    }
    x = filter_.Process(x);
    if (MovementDetector::Process(x))
      put({PotMove, INPUT});
    return x;                   // 0..1
  }
};

enum class Takeover { HARD, SOFT };

enum class PotFct { MAIN, ALT };

template<AdcInput INPUT, Law LAW, class FILTER, Takeover TO>
class DualFunctionPotConditioner : PotConditioner<INPUT, LAW, FILTER> {
  enum State { MAIN, ALT, ARMING, CATCHUP } state_ = MAIN;
  f main_value_;
  f alt_value_ = -1_f;          // -1 indicates no value
  f error_;
public:

  DualFunctionPotConditioner(Adc& adc) : PotConditioner<INPUT, LAW, FILTER>(adc) {}

  void alt() { state_ = ALT; }
  void main() { if (state_ == ALT) state_ = ARMING; }

  std::pair<f, f> Process(std::function<void(Event)> const& put) {
    f input = PotConditioner<INPUT, LAW, FILTER>::Process(put);
    switch(state_) {
    case MAIN: {
      main_value_ = input;
    } break;
    case ALT: {
      alt_value_ = input;
    } break;
    case ARMING: {
      error_ = input - main_value_;
      state_ = CATCHUP;
      put({StartCatchup, INPUT});
    } break;
    case CATCHUP: {
      if (TO == Takeover::HARD) {
        if ((input - alt_value_).abs() > kPotMoveThreshold) {
          state_ = MAIN;
          put({EndOfCatchup, INPUT});
        }
      } else if (TO == Takeover::SOFT) {
        // end of catchup happens if errors don't have the same sign
        // (the knob crossed its previous recorded value). The small
        // constant is to avoid being stuck in catchup when the error
        // is close to zero (pot didn't move) or if main_value_ = 0.
        if ((input - main_value_) * error_ <= 0.0001_f) {
          state_ = MAIN;
          put({EndOfCatchup, INPUT});
        }
      }
    }
    }
    return std::pair(main_value_, alt_value_);
  }
};

template<class PotConditioner, class CVConditioner, class FILTER>
class PotCVCombiner {
  FILTER filter_;

public:
  PotConditioner pot_;
  CVConditioner cv_;

  PotCVCombiner(Adc& adc, f cv_offset) : pot_(adc), cv_(adc, cv_offset) {}

  // TODO disable this function if PotConditioner = DualFunction
  f Process(std::function<void(Event)> const& put) {
    f x = pot_.Process(put);
    x -= cv_.Process();
    x = x.clip(0_f, 1_f);
    return filter_.Process(x);
  }

  std::pair<f, f> ProcessDualFunction(std::function<void(Event)> const& put) {
    auto [main, alt] = pot_.Process(put);

    // sum main pot function and its associated CV
    main -= cv_.Process();
    main = main.clip(0_f, 1_f);
    main = filter_.Process(main);

    return std::pair(main, alt);
  }

  f last() { return filter_.last(); }
};

struct NoFilter {
  f Process(f x) { return x; }
};

template<int block_size>
class Control : public EventSource<Event> {

  Adc adc_;

  PotCVCombiner<PotConditioner<POT_DETUNE, Law::LINEAR, NoFilter>,
                NoCVInput, QuadraticOnePoleLp<1>> detune_ {adc_, 0_f};
  PotCVCombiner<DualFunctionPotConditioner<POT_WARP, Law::LINEAR,
                                           QuadraticOnePoleLp<1>, Takeover::SOFT>,
                CVConditioner<CV_WARP>, QuadraticOnePoleLp<1>> warp_ {adc_, 0.000183111057_f};
  PotCVCombiner<DualFunctionPotConditioner<POT_TILT, Law::LINEAR,
                                           QuadraticOnePoleLp<1>, Takeover::SOFT>,
                CVConditioner<CV_TILT>, QuadraticOnePoleLp<1>> tilt_ {adc_, 0.000793481246_f};
  PotCVCombiner<DualFunctionPotConditioner<POT_TWIST, Law::LINEAR,
                                           QuadraticOnePoleLp<1>, Takeover::SOFT>,
                CVConditioner<CV_TWIST>, QuadraticOnePoleLp<1>> twist_ {adc_, 0.00189214759_f};
  PotCVCombiner<PotConditioner<POT_GRID, Law::LINEAR, NoFilter>,
                CVConditioner<CV_GRID>, QuadraticOnePoleLp<1>> grid_ {adc_, 0.00146488845_f};
  PotCVCombiner<PotConditioner<POT_MOD, Law::LINEAR, NoFilter>,
                CVConditioner<CV_MOD>, QuadraticOnePoleLp<1>> mod_ {adc_, -0.00106814783_f};
  PotCVCombiner<DualFunctionPotConditioner<POT_SPREAD, Law::LINEAR,
                                           QuadraticOnePoleLp<1>, Takeover::SOFT>,
                CVConditioner<CV_SPREAD>, QuadraticOnePoleLp<1>> spread_ {adc_, 0.00129703665_f};

  DualFunctionPotConditioner<POT_PITCH, Law::LINEAR,
                             QuadraticOnePoleLp<2>, Takeover::SOFT> pitch_pot_ {adc_};
  DualFunctionPotConditioner<POT_ROOT, Law::LINEAR,
                             QuadraticOnePoleLp<2>, Takeover::SOFT> root_pot_ {adc_};
  AudioCVConditioner<block_size> pitch_cv_ {0.240466923_f, 96.8885345_f};
  AudioCVConditioner<block_size> root_cv_  {0.24319829_f, 97.4769897_f};

  Parameters& params_;

  Sampler<f> pitch_cv_sampler_;

public:

  Control(Parameters& params) :
    params_(params) {}

  void ProcessCodecInput(Buffer<Frame, block_size>& codec_in) {

    // Process codec input
    Buffer<s1_15, block_size> pitch_block;
    Buffer<s1_15, block_size> root_block;

    for (auto [in, pi, ro] : zip(codec_in, pitch_block, root_block)) {
      pi = in.l;
      ro = in.r;
    }

    pitch_cv_.Process(pitch_block);
    root_cv_.Process(root_block);
  }

  void Poll(std::function<void(Event)> const& put) {

    // Process potentiometer & CV

    // DETUNE
    { f detune = detune_.Process(put);
      detune = (detune * detune) * (detune * detune);
      detune *= 10_f / f(kMaxNumOsc);
      params_.detune = detune;
    }

    // TILT
    { auto [tilt, crossfade] = tilt_.ProcessDualFunction(put);

      tilt = tilt * 2_f - 1_f; // -1..1
      tilt *= tilt * tilt;     // -1..1 cubic
      tilt *= 4_f;             // -4..4
      tilt = Math::fast_exp2(tilt); // 0.0625..16
      params_.tilt = tilt;

      if (crossfade > 0_f) {
        crossfade *= crossfade;
        crossfade = 1_f - crossfade;
        crossfade *= 0.5_f;
        params_.crossfade_factor = crossfade; // 0..1
      }
    }

    // TWIST
    { auto [twist, freeze_mode] = twist_.ProcessDualFunction(put);

      // avoids CV noise to produce harmonics near 0
      twist = Signal::crop_down(0.01_f, twist);
      // scaling of Twist
      if (params_.twist.mode == FEEDBACK) {
        twist *= twist * 0.7_f;
      } else if (params_.twist.mode == PULSAR) {
        twist *= twist;
        twist = Math::fast_exp2(twist * 7_f); // 0..2^7
      } else if (params_.twist.mode == DECIMATE) {
        twist *= twist * 0.5_f;
      }
      params_.twist.value = twist;

      if (freeze_mode > 0_f) {
        freeze_mode *= 3_f;           // 3 split modes
        SplitMode m = static_cast<SplitMode>(freeze_mode.floor());
        if (m != params_.freeze_mode) put({AltParamChange, m});
        params_.freeze_mode = m;
      }
    }

    // WARP
    { auto [warp, stereo_mode] = warp_.ProcessDualFunction(put);

      // avoids CV noise to produce harmonics near 0
      warp = Signal::crop_down(0.01_f, warp);
      if (params_.warp.mode == FOLD) {
        warp *= warp;
        warp *= 0.9_f;
        // this little offset avoids scaling the input too close to
        // zero; reducing it makes the wavefolder more linear around
        // warp=0, but increases the quantization noise.
        warp += 0.004_f;
      } else if (params_.warp.mode == CHEBY) {
      } else if (params_.warp.mode == CRUSH) {
      }
      params_.warp.value = warp;

      if (stereo_mode > 0_f) {
        stereo_mode *= 3_f;           // 3 split modes
        SplitMode m = static_cast<SplitMode>(stereo_mode.floor());
        if (m != params_.stereo_mode) put({AltParamChange, m});
        params_.stereo_mode = m;
      }
    }

    // MODULATION
    { f mod = mod_.Process(put);
      // avoids CV noise to produce harmonics near 0
      mod = Signal::crop_down(0.01_f, mod);
      mod *= 4_f / f(params_.numOsc);
      if (params_.modulation.mode == ONE) {
        mod *= 0.9_f;
      } else if (params_.modulation.mode == TWO) {
        mod *= 6.0_f;
      } else if (params_.modulation.mode == THREE) {
        mod *= 4.0_f;
      }
      params_.modulation.value = mod;
    }

    // SPREAD
    { auto [spread, numOsc] = spread_.ProcessDualFunction(put);

      spread *= 10_f / f(kMaxNumOsc);
      params_.spread = spread * kSpreadRange;

      if (numOsc > 0_f) {
        numOsc *= f(kMaxNumOsc-1); // [0..max]
        numOsc += 1.5_f;           // [1.5..max+0.5]
        int n = numOsc.floor();    // [1..max]
        if (n != params_.numOsc) put({AltParamChange, n});
        params_.numOsc = n;
      }
    }

    // GRID
    { f grid = grid_.Process(put);
      grid *= 9_f;                           // [0..9]
      grid += 0.5_f;                         // [0.5..9.5]
      int g = grid.floor();
      if (g != params_.grid.value) put({GridChange, g});
      params_.grid.value = g; // [0..9]
    }

    // PITCH
    { auto [pitch, fine_tune] = pitch_pot_.Process(put);
      pitch *= kPitchPotRange;                               // 0..range
      pitch -= kPitchPotRange * 0.5_f;                       // -range/2..range/2
      f pitch_cv = pitch_cv_.last();
      pitch_cv = pitch_cv_sampler_.Process(pitch_cv);
      pitch += pitch_cv;
      params_.pitch = pitch;

      if (fine_tune > 0_f) {
        params_.fine_tune = (fine_tune - 0.5_f) * kNewNoteFineRange;
      }
    }

    // ROOT
    { auto [root, new_note] = root_pot_.Process(put);
      root *= kRootPotRange;
      root += root_cv_.last();
      params_.root = root.max(0_f);

      if (new_note > 0_f) {
        params_.new_note = new_note * kNewNoteRange + kNewNoteRange * 0.5_f;
      }
    }
  }

  f pitch_cv() { return pitch_cv_.last(); }
  void hold_pitch_cv() { pitch_cv_sampler_.hold(); }
  void release_pitch_cv() { pitch_cv_sampler_.release(); }

  void spread_pot_alternate_function() { spread_.pot_.alt(); }
  void spread_pot_main_function() { spread_.pot_.main(); }

  void root_pot_alternate_function() { root_pot_.alt(); }
  void root_pot_main_function() { root_pot_.main(); }
  void pitch_pot_alternate_function() { pitch_pot_.alt(); }
  void pitch_pot_main_function() { pitch_pot_.main(); }
  void twist_pot_alternate_function() { twist_.pot_.alt(); }
  void twist_pot_main_function() { twist_.pot_.main(); }
  void warp_pot_alternate_function() { warp_.pot_.alt(); }
  void warp_pot_main_function() { warp_.pot_.main(); }
  void tilt_pot_alternate_function() { tilt_.pot_.alt(); }
  void tilt_pot_main_function() { tilt_.pot_.main(); }

  void all_main_function() {
    spread_pot_main_function();
    root_pot_main_function();
    pitch_pot_main_function();
    twist_pot_main_function();
    warp_pot_main_function();
    tilt_pot_main_function();
  }

  bool CalibrateOffset() {
    return pitch_cv_.calibrate_offset()
      && root_cv_.calibrate_offset()
      && warp_.cv_.calibrate_offset()
      && tilt_.cv_.calibrate_offset()
      && twist_.cv_.calibrate_offset()
      && grid_.cv_.calibrate_offset()
      && mod_.cv_.calibrate_offset()
      && spread_.cv_.calibrate_offset();
  }
  bool CalibrateSlope() {
    return pitch_cv_.calibrate_slope()
      && root_cv_.calibrate_slope();
  }
};
