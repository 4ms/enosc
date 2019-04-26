#pragma once

#include "adc.hh"
#include "dsp.hh"
#include "polyptic_oscillator.hh"
#include "event_handler.hh"

const int kPotFiltering = 1;     // 0..16
const f kPotDeadZone = 0.01_f;
const f kPitchPotRange = 6_f * 12_f;
const f kRootPotRange = 10_f * 12_f;
const f kSpreadRange = 12_f;
const f kCalibration2Voltage = 4_f;
const f kCalibrationSuccessTolerance = 0.2_f;
const u0_16 kPotMoveThreshold = 0.02_u0_16;

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

  void Process(Block<s1_15, block_size> in) {
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

enum Law { LINEAR, QUADRATIC, CUBIC, QUARTIC };

template<AdcInput input, Law LAW>  // Lp = 0..16
class PotConditioner {
  u0_16 previous_value_;
  Adc& adc_;
public:
  PotConditioner(Adc& adc) : adc_(adc) {}
  u0_16 Process(std::function<void(Event)> put) {
    u0_16 x = adc_.get(input);
    switch(LAW) {
    case LINEAR: break;
    case QUADRATIC: x = u0_16::narrow(x * x); break;
    case CUBIC: x = u0_16::narrow(u0_16::narrow(x * x) * x); break;
    case QUARTIC:
      x = u0_16::narrow(x * x);
      x = u0_16::narrow(x * x);
      break;
    }
    u0_16 diff = x - previous_value_;
    if (diff > 0.5_u0_16) diff = 1._u0_16 - diff;
    if (diff > kPotMoveThreshold) {
      put({PotMoved, input});
      previous_value_ = x;
    }
    return x;
  }
};

template<AdcInput input>
class CVConditioner {
  Adc& adc_;

public:
  CVConditioner(Adc& adc) : adc_(adc) {}

  s1_15 Process() {
    u0_16 in = adc_.get(input);
    // TODO calibration
    s1_15 x = in.to_signed_scale();
    return x;                 // -1..1
  }
};

struct NoCVInput {
  NoCVInput(Adc& adc) {}
  s1_15 Process() { return 0._s1_15; }
};

template<class PotConditioner, class CVConditioner, int LP>
class PotCVCombiner {
  Adc& adc_;
  PotConditioner pot_ {adc_};
  CVConditioner cv_ {adc_};
  QuadraticOnePoleLp<LP> lp_;

public:
  PotCVCombiner(Adc& adc) : adc_(adc) {}

  f Process(std::function<void(Event)> put) {
    s17_15 x = s17_15(pot_.Process(put).to_signed());
    // TODO use saturating add
    x -= s17_15(cv_.Process());
    f y = x.to_float_inclusive().clip(0_f, 1.0_f);
    return lp_.Process(y);
  }

  f last() { return lp_.last(); }
};

template<int block_size>
class Control : public EventSource<Event> {

  Adc adc_;

  PotCVCombiner<PotConditioner<DETUNE_POT, LINEAR>,
                NoCVInput, kPotFiltering> detune_ {adc_};
  PotCVCombiner<PotConditioner<WARP_POT, LINEAR>,
                CVConditioner<WARP_CV>, kPotFiltering> warp_ {adc_};
  PotCVCombiner<PotConditioner<TILT_POT, LINEAR>,
                CVConditioner<TILT_CV>, kPotFiltering> tilt_ {adc_};
  PotCVCombiner<PotConditioner<TWIST_POT, LINEAR>,
                CVConditioner<TWIST_CV>, kPotFiltering> twist_ {adc_};
  PotCVCombiner<PotConditioner<GRID_POT, LINEAR>,
                CVConditioner<GRID_CV>, kPotFiltering> grid_ {adc_};
  PotCVCombiner<PotConditioner<MOD_POT, LINEAR>,
                CVConditioner<MOD_CV>, kPotFiltering> mod_ {adc_};
  PotCVCombiner<PotConditioner<SPREAD_POT, LINEAR>,
                CVConditioner<SPREAD_CV>, kPotFiltering> spread_ {adc_};

  PotConditioner<PITCH_POT, LINEAR> pitch_pot_ {adc_};
  PotConditioner<ROOT_POT, LINEAR> root_pot_ {adc_};
  QuadraticOnePoleLp<2> pitch_pot_lp_;
  QuadraticOnePoleLp<2> root_pot_lp_;
  AudioCVConditioner<block_size> pitch_cv_ {0.240466923_f, 96.8885345_f};
  AudioCVConditioner<block_size> root_cv_  {0.24319829_f, 97.4769897_f};

  PolypticOscillator<block_size> &osc_;
  Parameters& params_;

  Sampler<f> pitch_cv_sampler_;

public:

  Control(PolypticOscillator<block_size> &osc, Parameters& params) :
    osc_(osc), params_(params) {}

  void ProcessCodecInput(Block<Frame, block_size> codec_in) {

    // Process codec input
    s1_15 in1[block_size], in2[block_size];
    Block<s1_15, block_size> pitch_block {in1};
    Block<s1_15, block_size> root_block {in2};

    for (auto [in, pi, ro] : zip(codec_in, pitch_block, root_block)) {
      pi = in.l;
      ro = in.r;
    }

    pitch_cv_.Process(pitch_block);
    root_cv_.Process(root_block);
  }

  void Poll(std::function<void(Event)> put) {

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
    spread *= spread;
    params_.spread = spread * kSpreadRange;

    f grid = grid_.Process(put);
    grid = Signal::crop(kPotDeadZone, grid); // [0..1]
    grid *= 9_f;                           // [0..9]
    grid += 0.5_f;                         // [0.5..9.5]
    int g = grid.floor();
    if (g != params_.grid.value) put({GridChanged, 0});
    params_.grid.value = g; // [0..9]

    // Root & Pitch
    u0_16 r = root_pot_.Process(put);
    f root = root_pot_lp_.Process(r.to_float_inclusive());
    root *= kRootPotRange;
    root += root_cv_.last();
    params_.root = root.max(0_f);

    u0_16 p = pitch_pot_.Process(put);
    f pitch = pitch_pot_lp_.Process(p.to_float_inclusive()); // 0..1
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

  bool CalibrateOffset() {
    return pitch_cv_.calibrate_offset()
      && root_cv_.calibrate_offset();
  }
  bool CalibrateSlope() {
    return pitch_cv_.calibrate_slope()
      && root_cv_.calibrate_slope();
  }
};
