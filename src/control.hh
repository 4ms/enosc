#pragma once

#include "adc.hh"
#include "dsp.hh"
#include "polyptic_oscillator.hh"
#include "event_handler.hh"

const f kPotDeadZone = 0.01_f;
const f kPitchPotRange = 6_f * 12_f;
const f kRootPotRange = 10_f * 12_f;
const f kSpreadRange = 12_f;
const f kCalibration2Voltage = 4_f;
const f kCalibrationSuccessTolerance = 0.2_f;
const f kPotMoveThreshold = 0.01_f;

template<int block_size>
class AudioCVConditioner {
  Average<8, 1> lp_;
  CicDecimator<1, block_size> cic_;
  f offset_, nominal_offset_;
  f slope_, nominal_slope_;
  f last_;
  f last_raw_reading() { return lp_.last().to_float_inclusive(); }
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
    last_ = y.to_float_inclusive(); // 0..1
    last_ -= offset_;
    last_ *= slope_;                // -24..72
  }

  f last() { return last_; }
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
    f x = adc_.get(INPUT).to_float_inclusive();
    switch(LAW) {
    case Law::LINEAR: break;
    case Law::QUADRATIC: x = x * x; break;
    case Law::CUBIC: x = x * x * x; break;
    case Law::QUARTIC: x = x * x; x = x * x; break;
    }
    x = filter_.Process(x);
    if (MovementDetector::Process(x))
      put({PotMove, INPUT});
    return x;
  }
};

enum class Takeover { HARD, SOFT };

enum class PotFct { MAIN, ALT };

template<AdcInput INPUT, Law LAW, class FILTER, Takeover TO>
class DualFunctionPotConditioner : PotConditioner<INPUT, LAW, FILTER> {
  enum State { MAIN, ALT, ARMING, CATCHUP } state_ = MAIN;
  f main_value_;
  f alt_value_;
  bool sgn_;
public:

  DualFunctionPotConditioner(Adc& adc) : PotConditioner<INPUT, LAW, FILTER>(adc) {}

  void alt() { state_ = ALT; }
  void main() { if (state_ == ALT) state_ = ARMING; }

  std::pair<PotFct, f> Process(std::function<void(Event)> const& put) {
    f input = PotConditioner<INPUT, LAW, FILTER>::Process(put);
    switch(state_) {
    case MAIN: main_value_ = input; return std::pair(PotFct::MAIN, input);
    case ALT: alt_value_ = input; return std::pair(PotFct::ALT, input);
    case ARMING: {
      sgn_ = input - main_value_ > 0_f;
      state_ = CATCHUP;
      put({StartCatchup, INPUT});
      return std::pair(PotFct::ALT, input);
    } break;
    case CATCHUP: {
      switch (TO) {
      case Takeover::HARD: {
        if ((input - alt_value_).abs() > kPotMoveThreshold) {
          state_ = MAIN;
          put({EndOfCatchup, INPUT});
        }
      } break;
      case Takeover::SOFT: {
        if ((input - main_value_ > 0_f) != sgn_) {
          state_ = MAIN;
          put({EndOfCatchup, INPUT});
        }
      } break;
      }
    } break;
    }
    return std::pair(PotFct::MAIN, main_value_);
  }
};

template<AdcInput INPUT>
class CVConditioner {
  Adc& adc_;

public:
  CVConditioner(Adc& adc) : adc_(adc) {}

  f Process() {
    u0_16 in = adc_.get(INPUT);
    // TODO calibration
    s1_15 x = in.to_signed_scale();
    return x.to_float_inclusive();
  }
};

struct NoCVInput {
  NoCVInput(Adc& adc) {}
  f Process() { return 0._f; }
};

template<class PotConditioner, class CVConditioner, class FILTER>
class PotCVCombiner {
  Adc& adc_;
  FILTER filter_;

public:
  PotConditioner pot_ {adc_};
  CVConditioner cv_ {adc_};

  PotCVCombiner(Adc& adc) : adc_(adc) {}

  // TODO disable this function if PotCondition = DualFunction
  f Process(std::function<void(Event)> const& put) {
    f x = pot_.Process(put);
    x -= cv_.Process();
    x = x.clip(0_f, 1_f);
    return filter_.Process(x);
  }

  std::pair<PotFct, f> ProcessDualFunction(std::function<void(Event)> const& put) {
    auto [fct, x] = pot_.Process(put);
    if (fct == PotFct::MAIN) {
      x -= cv_.Process();
      x = x.clip(0_f, 1_f);
      x = filter_.Process(x);
    }
    return std::pair(fct, x);
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
                NoCVInput, QuadraticOnePoleLp<1>> detune_ {adc_};
  PotCVCombiner<PotConditioner<POT_WARP, Law::LINEAR, NoFilter>,
                CVConditioner<CV_WARP>, QuadraticOnePoleLp<1>> warp_ {adc_};
  PotCVCombiner<PotConditioner<POT_TILT, Law::LINEAR, NoFilter>,
                CVConditioner<CV_TILT>, QuadraticOnePoleLp<1>> tilt_ {adc_};
  PotCVCombiner<PotConditioner<POT_TWIST, Law::LINEAR, NoFilter>,
                CVConditioner<CV_TWIST>, QuadraticOnePoleLp<1>> twist_ {adc_};
  PotCVCombiner<
    DualFunctionPotConditioner<POT_GRID, Law::LINEAR,
                               QuadraticOnePoleLp<1>, Takeover::SOFT>,
                CVConditioner<CV_GRID>, QuadraticOnePoleLp<1>> grid_ {adc_};
  PotCVCombiner<PotConditioner<POT_MOD, Law::LINEAR, NoFilter>,
                CVConditioner<CV_MOD>, QuadraticOnePoleLp<1>> mod_ {adc_};
  PotCVCombiner<PotConditioner<POT_SPREAD, Law::LINEAR, NoFilter>,
                CVConditioner<CV_SPREAD>, QuadraticOnePoleLp<1>> spread_ {adc_};

  PotConditioner<POT_PITCH, Law::LINEAR, QuadraticOnePoleLp<2>> pitch_pot_ {adc_};
  PotConditioner<POT_ROOT, Law::LINEAR, QuadraticOnePoleLp<2>> root_pot_ {adc_};
  AudioCVConditioner<block_size> pitch_cv_ {0.240466923_f, 96.8885345_f};
  AudioCVConditioner<block_size> root_cv_  {0.24319829_f, 97.4769897_f};

  Parameters& params_;

  Sampler<f> pitch_cv_sampler_;

  bool new_note_tracking;

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

    f detune = detune_.Process(put);
    detune = Signal::crop_down(kPotDeadZone, detune);
    detune = (detune * detune) * (detune * detune);
    params_.detune = detune;

    f tilt = tilt_.Process(put);
    tilt = Signal::crop(kPotDeadZone, tilt);
    tilt = tilt * 2_f - 1_f;
    tilt *= tilt * tilt;
    tilt *= 4_f;
    tilt = Math::fast_exp2(tilt);
    params_.tilt = tilt;

    f twist = twist_.Process(put);
    twist = Signal::crop(kPotDeadZone, twist);
    if (params_.twist.mode == FEEDBACK) {
      twist *= twist * 0.7_f;
    } else if (params_.twist.mode == PULSAR) {
      twist *= twist;
      twist = Math::fast_exp2(twist * 7_f);
      // twist: 0..2^7
    } else if (params_.twist.mode == DECIMATE) {
      twist *= twist * 0.5_f;
    }
    params_.twist.value = twist;

    f warp = warp_.Process(put);
    warp = Signal::crop(kPotDeadZone, warp);
    if (params_.warp.mode == FOLD) {
      warp *= warp;
      warp *= 0.9_f;
      warp += 0.01_f;
    } else if (params_.warp.mode == CHEBY) {
    } else if (params_.warp.mode == CRUSH) {
    }
    params_.warp.value = warp;

    f mod = mod_.Process(put);
    mod = Signal::crop(kPotDeadZone, mod);
    mod *= 4_f / f(params_.numOsc);
    if (params_.modulation.mode == ONE) {
      mod *= 0.9_f;
    } else if (params_.modulation.mode == TWO) {
      mod *= 6.0_f;
    } else if (params_.modulation.mode == THREE) {
      mod *= 4.0_f;
    }
    params_.modulation.value = mod;

    f spread = spread_.Process(put);
    spread = Signal::crop(kPotDeadZone, spread);
    params_.spread = spread * kSpreadRange;

    auto [fct, grid] = grid_.ProcessDualFunction(put);
    grid = Signal::crop(kPotDeadZone, grid); // [0..1]
    if (fct == PotFct::MAIN) {
      grid *= 9_f;                           // [0..9]
      grid += 0.5_f;                         // [0.5..9.5]
      int g = grid.floor();
      if (g != params_.grid.value) put({GridChange, g});
      params_.grid.value = g; // [0..9]
    } else {
      grid *= f(kMaxNumOsc-1); // [0..max+1]
      grid += 1.5_f;           // [1.5..max+0.5]
      int n = grid.floor();    // [1..max]
      if (n != params_.numOsc) put({NumOscChange, n});
      params_.numOsc = n;
    }

    // Root & Pitch
    f root = root_pot_.Process(put);
    root *= kRootPotRange;
    root += root_cv_.last();
    params_.root = root.max(0_f);

    if (new_note_tracking) {
      params_.new_note = params_.root;
    }

    f pitch = pitch_pot_.Process(put);
    pitch *= kPitchPotRange;                               // 0..range
    pitch -= kPitchPotRange * 0.5_f;                       // -range/2..range/2

    f pitch_cv = pitch_cv_.last();
    pitch_cv = pitch_cv_sampler_.Process(pitch_cv);
    pitch += pitch_cv;
    params_.pitch = pitch;


    // Start next conversion
    adc_.Start();
  }

  f pitch_cv() { return pitch_cv_.last(); }
  void hold_pitch_cv() { pitch_cv_sampler_.hold(); }
  void release_pitch_cv() { pitch_cv_sampler_.release(); }

  void grid_pot_alternate_function() { grid_.pot_.alt(); }
  void grid_pot_main_function() { grid_.pot_.main(); }

  void all_main_function() {
    grid_pot_main_function();
  }

  void set_new_note_tracking(bool b) { new_note_tracking = b; }

  bool CalibrateOffset() {
    return pitch_cv_.calibrate_offset()
      && root_cv_.calibrate_offset();
  }
  bool CalibrateSlope() {
    return pitch_cv_.calibrate_slope()
      && root_cv_.calibrate_slope();
  }
};
