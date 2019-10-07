#pragma once

#include "adc.hh"
#include "spi_adc.hh"
#include "dsp.hh"
#include "event_handler.hh"
#include "persistent_storage.hh"
#include "qspi_flash.hh"

const f kPotDeadZone = 0.01_f;
const f kPitchPotRange = 6_f * 12_f;
const f kRootPotRange = 9_f * 12_f;
const f kNewNoteRange = 6_f * 12_f;
const f kNewNoteFineRange = 4_f;
const f kSpreadRange = 12_f;
const f kCalibration4Volts = 4_f;
const f kCalibrationSuccessTolerance = 0.3_f;
const f kCalibrationSuccessToleranceOffset = 0.1_f;
const f kPotMoveThreshold = 0.01_f;

const int kCalibrationIterations = 16;

template<int CHAN, class FILTER>
class ExtCVConditioner {
  SpiAdc& spi_adc_;
  f& offset_;
  f nominal_offset_;
  f& slope_;
  f nominal_slope_;
  FILTER lp_;

  f last_raw_reading() { return f::inclusive(lp_.last()); }
  
public:
  ExtCVConditioner(f& o, f& s, SpiAdc& spi_adc) :
    offset_(o), nominal_offset_(o),
    slope_(s), nominal_slope_(s),
    spi_adc_(spi_adc) {}

  bool calibrate_offset() {
    f offset = last_raw_reading();
    if ((offset - nominal_offset_).abs() < kCalibrationSuccessToleranceOffset) {
      offset_ = offset;
      return true;
    } else return false;
  }

  bool calibrate_slope() {
    f octave = (last_raw_reading() - offset_) / kCalibration4Volts;
    f slope = 12_f / octave;

    if ((slope / nominal_slope_ - 1_f).abs() < kCalibrationSuccessTolerance) {
      slope_ = slope;
      return true;
    } else return false;
  }

  void Process() {
    u0_16 x = spi_adc_.get(CHAN);
    lp_.Process(x);
  }

  f last() {
    return (f::inclusive(lp_.last()) - offset_) * slope_;
  }
};

template<AdcInput INPUT>
class CVConditioner {
  Adc& adc_;
  f& offset_;

public:
  CVConditioner(Adc& adc, f& offset) : adc_(adc), offset_(offset) {}

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
  NoCVInput(Adc& adc) {}
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
  
  f raw() { return f::inclusive(adc_.get(INPUT)); }

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
class DualFunctionPotConditioner : public PotConditioner<INPUT, LAW, FILTER> {
  enum State { MAIN, ALT, ARMING, CATCHUP } state_ = MAIN;
  f main_value_;
  f alt_value_ = -1_f;          // -1 indicates no value
  f error_;
public:

  DualFunctionPotConditioner(Adc& adc) : PotConditioner<INPUT, LAW, FILTER>(adc) {}

  void alt() { state_ = ALT; }
  void main() { if (state_ == ALT) state_ = ARMING; }
  void reset_alt_value() { alt_value_ = -1_f; }

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

  PotCVCombiner(Adc& adc) : pot_(adc), cv_(adc) {}
  PotCVCombiner(Adc& adc, f& cv_offset) : pot_(adc), cv_(adc, cv_offset) {}

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
  SpiAdc spi_adc_;

  struct CalibrationData {
    f pitch_offset;
    f pitch_slope;
    f root_offset;
    f root_slope;
    f warp_offset;
    f balance_offset;
    f twist_offset;
    f scale_offset;
    f modulation_offset;
    f spread_offset;

    // on load, checks that calibration data are within bounds
    bool validate() {
      return
        (pitch_offset - 0.70_f).abs() <= kCalibrationSuccessToleranceOffset &&
        (pitch_slope / -110._f - 1_f).abs() <= kCalibrationSuccessTolerance &&
        pitch_slope < 0.0_f &&
        (root_offset - 0.70_f).abs() <= kCalibrationSuccessToleranceOffset &&
        (root_slope / -110._f - 1_f).abs() <= kCalibrationSuccessTolerance &&
        root_slope < 0.0_f &&
        warp_offset.abs() <= kCalibrationSuccessToleranceOffset &&
        balance_offset.abs() <= kCalibrationSuccessToleranceOffset &&
        twist_offset.abs() <= kCalibrationSuccessToleranceOffset &&
        scale_offset.abs() <= kCalibrationSuccessToleranceOffset &&
        modulation_offset.abs() <= kCalibrationSuccessToleranceOffset &&
        spread_offset.abs() <= kCalibrationSuccessToleranceOffset;
    }
  };
  CalibrationData calibration_data_;
  CalibrationData default_calibration_data_ = {
    0.700267_f, -110.17_f, //pitch_offset, slope
    0.697154_f, -110.355_f, //root_offset, slope
    0.00302133_f, //warp_offset
    0.00476089_f, //balance_offset
    0.00177007_f, //twist_offset
    0.00146489_f, //scale_offset
    0.00265511_f, //modulation_offset
    0.00326548_f, //spread_offset
  };

  Persistent<WearLevel<FlashBlock<0, CalibrationData>>>
  calibration_data_storage_ {&calibration_data_, default_calibration_data_};

  PotCVCombiner<PotConditioner<POT_DETUNE, Law::LINEAR, NoFilter>,
                NoCVInput, QuadraticOnePoleLp<1>
                > detune_ {adc_};
  PotCVCombiner<DualFunctionPotConditioner<POT_WARP, Law::LINEAR,
                                           QuadraticOnePoleLp<1>, Takeover::SOFT>,
                CVConditioner<CV_WARP>, QuadraticOnePoleLp<1>
                > warp_ {adc_, calibration_data_.warp_offset};
  PotCVCombiner<DualFunctionPotConditioner<POT_BALANCE, Law::LINEAR,
                                           QuadraticOnePoleLp<1>, Takeover::SOFT>,
                CVConditioner<CV_BALANCE>, QuadraticOnePoleLp<2>
                > balance_ {adc_, calibration_data_.balance_offset};
  PotCVCombiner<DualFunctionPotConditioner<POT_TWIST, Law::LINEAR,
                                           QuadraticOnePoleLp<1>, Takeover::SOFT>,
                CVConditioner<CV_TWIST>, QuadraticOnePoleLp<1>
                > twist_ {adc_, calibration_data_.twist_offset};
  PotCVCombiner<PotConditioner<POT_SCALE, Law::LINEAR, NoFilter>,
                CVConditioner<CV_SCALE>, QuadraticOnePoleLp<1>
                > scale_ {adc_, calibration_data_.twist_offset};
  PotCVCombiner<PotConditioner<POT_MOD, Law::LINEAR, NoFilter>,
                CVConditioner<CV_MOD>, QuadraticOnePoleLp<1>
                > modulation_ {adc_, calibration_data_.modulation_offset};
  PotCVCombiner<DualFunctionPotConditioner<POT_SPREAD, Law::LINEAR,
                                           QuadraticOnePoleLp<1>, Takeover::SOFT>,
                CVConditioner<CV_SPREAD>, QuadraticOnePoleLp<1>
                > spread_ {adc_, calibration_data_.spread_offset};

  DualFunctionPotConditioner<POT_PITCH, Law::LINEAR,
                             QuadraticOnePoleLp<2>, Takeover::SOFT
                             > pitch_pot_ {adc_};
  DualFunctionPotConditioner<POT_ROOT, Law::LINEAR,
                             QuadraticOnePoleLp<2>, Takeover::SOFT
                             > root_pot_ {adc_};
  ExtCVConditioner<CV_PITCH, Average<4, 2>
                   > pitch_cv_ {calibration_data_.pitch_offset,
                                calibration_data_.pitch_slope, 
                                spi_adc_};
  ExtCVConditioner<CV_ROOT, Average<4, 2>
                   > root_cv_ {calibration_data_.root_offset,
                               calibration_data_.root_slope, 
                               spi_adc_};

  Parameters& params_;

  Sampler<f> pitch_cv_sampler_;

public:

  Control(Parameters& params) :
    params_(params) {}

  void ProcessSpiAdcInput() {
    pitch_cv_.Process();
    root_cv_.Process();
  }

  void Poll(std::function<void(Event)> const& put) {

    // Process potentiometer & CV

    // DETUNE
    { f detune = detune_.Process(put);
      detune = (detune * detune) * (detune * detune);
      detune *= 10_f / f(kMaxNumOsc);
      params_.detune = detune;
    }

    // BALANCE
    { auto [balance, crossfade] = balance_.ProcessDualFunction(put);

      balance = balance * 2_f - 1_f; // -1..1
      balance *= balance * balance;     // -1..1 cubic
      balance *= 4_f;             // -4..4
      balance = Math::fast_exp2(balance); // 0.0625..16
      params_.balance = balance;

      if (crossfade > 0_f) {
        crossfade *= crossfade;
        crossfade = 1_f - crossfade;
        crossfade *= 0.5_f;
        params_.alt.crossfade_factor = crossfade; // 0..1
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
        // Warning: when changing this constant, also change Pulsar
        // distortion function
        twist = Math::fast_exp2(twist * 6_f); // 1..2^6
      } else if (params_.twist.mode == CRUSH) {
        twist *= twist * 0.5_f;
      }
      params_.twist.value = twist;

      if (freeze_mode > 0_f) {
        freeze_mode *= 3_f;           // 3 split modes
        SplitMode m = static_cast<SplitMode>(freeze_mode.floor());
        if (m != params_.alt.freeze_mode) put({AltParamChange, m});
        params_.alt.freeze_mode = m;
      }
    }

    // WARP
    { auto [warp, stereo_mode] = warp_.ProcessDualFunction(put);

      // avoids CV noise to produce harmonics near 0
      warp = Signal::crop_down(0.01_f, warp);
      if (params_.warp.mode == FOLD) {
        warp *= warp;
        warp *= 0.9_f;
      } else if (params_.warp.mode == CHEBY) {
      } else if (params_.warp.mode == SEGMENT) {
      }
      params_.warp.value = warp;

      if (stereo_mode > 0_f) {
        stereo_mode *= 3_f;           // 3 split modes
        SplitMode m = static_cast<SplitMode>(stereo_mode.floor());
        if (m != params_.alt.stereo_mode) put({AltParamChange, m});
        params_.alt.stereo_mode = m;
      }
    }

    // MODULATION
    { f mod = modulation_.Process(put);
      // avoids CV noise to produce harmonics near 0
      mod = Signal::crop_down(0.01_f, mod);
      mod *= 6_f / f(params_.alt.numOsc);
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
        if (n != params_.alt.numOsc) put({AltParamChange, n});
        params_.alt.numOsc = n;
      }
    }

    // SCALE
    { f scale = scale_.Process(put);
      scale *= 9_f;                           // [0..9]
      scale += 0.5_f;                         // [0.5..9.5]
      int g = scale.floor();
      if (g != params_.scale.value) put({ScaleChange, g});
      params_.scale.value = g; // [0..9]
    }

    // PITCH
    { auto [pitch, fine_tune] = pitch_pot_.Process(put);
      pitch *= kPitchPotRange;                               // 0..range
      pitch -= kPitchPotRange * 0.5_f;                       // -range/2..range/2
      f pitch_cv = pitch_cv_.last();
      pitch_cv = pitch_cv_sampler_.Process(pitch_cv);
      pitch += pitch_cv;
      params_.pitch = pitch;

      params_.fine_tune =
        fine_tune > 0_f ? (fine_tune - 0.5_f) * kNewNoteFineRange : 0_f;
    }

    // ROOT
    { auto [root, new_note] = root_pot_.Process(put);
      root *= kRootPotRange;
      root += root_cv_.last();
      params_.root = root.max(0_f);

      if (new_note > 0_f) {
        new_note *= kRootPotRange;
        //Root CV is allowed to modify manually learned notes
        //so that if a keyboard/seq is patched into the jack, 
        //the learn'ed pitches are consistant.
        new_note += root_cv_.last();
        params_.new_note = new_note.max(0_f);
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
  void pitch_pot_reset_alternate_value() { pitch_pot_.reset_alt_value(); }
  void twist_pot_alternate_function() { twist_.pot_.alt(); }
  void twist_pot_main_function() { twist_.pot_.main(); }
  void warp_pot_alternate_function() { warp_.pot_.alt(); }
  void warp_pot_main_function() { warp_.pot_.main(); }
  void balance_pot_alternate_function() { balance_.pot_.alt(); }
  void balance_pot_main_function() { balance_.pot_.main(); }

  f scale_pot() { return scale_.pot_.raw(); }
  f balance_pot() { return balance_.pot_.raw(); }
  f twist_pot() { return twist_.pot_.raw(); }
  f pitch_pot() { return pitch_pot_.raw(); }
  f modulation_pot() { return modulation_.pot_.raw(); }
  f warp_pot() { return warp_.pot_.raw(); }

  void all_main_function() {
    spread_pot_main_function();
    root_pot_main_function();
    pitch_pot_main_function();
    twist_pot_main_function();
    warp_pot_main_function();
    balance_pot_main_function();
  }

  bool CalibrateOffset() {
    return pitch_cv_.calibrate_offset()
      && root_cv_.calibrate_offset()
      && warp_.cv_.calibrate_offset()
      && balance_.cv_.calibrate_offset()
      && twist_.cv_.calibrate_offset()
      && scale_.cv_.calibrate_offset()
      && modulation_.cv_.calibrate_offset()
      && spread_.cv_.calibrate_offset();
  }
  bool CalibrateSlope() {
    bool success = pitch_cv_.calibrate_slope()
      && root_cv_.calibrate_slope();
    if (success) calibration_data_storage_.Save();
    return success;
  }
};
