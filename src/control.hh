#pragma once

#include "adc.hh"
#include "spi_adc.hh"
#include "dsp.hh"
#include "event_handler.hh"
#include "persistent_storage.hh"
#include "qspi_flash.hh"
#include "gates.hh"
#include "polyptic_oscillator.hh"

const f kPotDeadZone = 0.01_f;
const f kPitchPotRange = 6_f * 12_f;
const f kRootPotRange = 9_f * 12_f;
const f kNewNoteRange = 6_f * 12_f;
const f kNewNoteFineRange = 4_f;
const f kSpreadRange = 12_f;
const f kCalibration2Volts = 2_f;
const f kCalibration4Volts = 4_f;
const f kCalibrationSuccessTolerance = 0.3_f;
const f kCalibrationSuccessToleranceOffset = 0.1_f;
const f kPotMoveThreshold = 0.01_f;

const int kIntCVCalibrationIterations = 16;
const int kExtCVCalibrationIterations = 1024;

enum CalibrationStepResult {
  CAL_STEP_RESULT_NOT_BEGUN,
  CAL_STEP_RESULT_IN_PROGRESS,
  CAL_STEP_RESULT_SUCCESS,
  CAL_STEP_RESULT_FAILURE,
};

enum CalibrationStep {
  CALIBRATE_UNPATCHED,
  CALIBRATE_C2,
  CALIBRATE_C4
};

template<int CHAN, class FILTER>
class ExtCVConditioner {
  SpiAdc& spi_adc_;
  f& offset_;
  f nominal_offset_;
  f& slope_;
  f nominal_slope_;
  FILTER lp_;

  f reading_at_C2;
  f reading_unpatched;
  int cal_i_;
  bool is_calibrating_=false;
  f cal_running_total_;

  CalibrationStep cal_step_;

  f last_raw_reading_;

public:
  ExtCVConditioner(f& o, f& s, SpiAdc& spi_adc) :
    offset_(o), nominal_offset_(o),
    slope_(s), nominal_slope_(s),
    spi_adc_(spi_adc) {}

  void reset_calibration() {
    is_calibrating_ = false;
    cal_step_ = CALIBRATE_UNPATCHED;
  }

  void start_calibration(CalibrationStep step) {
    cal_i_ = 0;
    cal_step_ = step;
    cal_running_total_ = 0_f;
    is_calibrating_ = true;
  }
  auto process_calibration() {
    CalibrationStepResult res;

    cal_running_total_ += last_raw_reading_;

    if (++cal_i_ >= kExtCVCalibrationIterations) {
      is_calibrating_ = false;
      cal_running_total_ /= f(kExtCVCalibrationIterations);

      switch (cal_step_) {

        case (CALIBRATE_UNPATCHED): {
          reading_unpatched = cal_running_total_;
          if ((reading_unpatched - nominal_offset_).abs() < kCalibrationSuccessToleranceOffset) {
            res = CAL_STEP_RESULT_SUCCESS;
           } else
            res = CAL_STEP_RESULT_FAILURE;
        } break;

        case (CALIBRATE_C2): {
          reading_at_C2 = cal_running_total_;
          f octave = (reading_at_C2 - reading_unpatched) / kCalibration2Volts;
          f slope = 12_f / octave;

          if ((slope / nominal_slope_ - 1_f).abs() < kCalibrationSuccessTolerance)
            res = CAL_STEP_RESULT_SUCCESS;
          else
            res = CAL_STEP_RESULT_FAILURE;
        } break;

        case (CALIBRATE_C4): {
          f octave = (cal_running_total_ - reading_at_C2) / kCalibration2Volts;
          f slope = 12_f / octave;

          if ((slope / nominal_slope_ - 1_f).abs() < kCalibrationSuccessTolerance) {
            slope_ = slope;
            offset_ = reading_at_C2 - (kCalibration2Volts * 12_f / slope_);
            res = CAL_STEP_RESULT_SUCCESS;
          } else
            res = CAL_STEP_RESULT_FAILURE;
        } break;

      }
    }
    else res = CAL_STEP_RESULT_IN_PROGRESS;

    return res;
  }

  bool calibration_busy() {
    return is_calibrating_;
  }

  void Process() {
    u0_16 x = spi_adc_.get(CHAN);
    lp_.Process(x);
  }

  void switch_channel() {
    spi_adc_.switch_channel();
  }

  f last() {
    last_raw_reading_ = f::inclusive(lp_.last());
    return (last_raw_reading_ - offset_) * slope_;
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

    for (int i=0; i<kIntCVCalibrationIterations; i++) {
      u0_16 in = adc_.get(INPUT);
      s1_15 x = in.to_signed_scale();
      reading += f::inclusive(x);
      HAL_Delay(1);
    }

    reading /= f(kIntCVCalibrationIterations);

    if (reading.abs() < kCalibrationSuccessToleranceOffset) {
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
  f cached_value_;
public:

  DualFunctionPotConditioner(Adc& adc) : PotConditioner<INPUT, LAW, FILTER>(adc) {}

  void alt() { state_ = ALT; }
  void main() { if (state_ == ALT) state_ = ARMING; }
  void reset_alt_value() { alt_value_ = -1_f; }
  void cache() { if (state_ == MAIN) cached_value_ = main_value_; }

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
        //Todo: set state_ to PLATEAUED_MAIN where the cached value is used
        //Then on movement detect set it to MAIN
        if ((input - cached_value_).abs() <= 0.0005_f) {
          state_ = MAIN;
          put({EndOfCatchup, INPUT});
        }
      }
    } break;
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
  Gates gates_;

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
        (pitch_slope / -112._f - 1_f).abs() <= kCalibrationSuccessTolerance &&
        pitch_slope < 0.0_f &&
        (root_offset - 0.70_f).abs() <= kCalibrationSuccessToleranceOffset &&
        (root_slope / -112._f - 1_f).abs() <= kCalibrationSuccessTolerance &&
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
    0.704_f, -112.73_f, //pitch_offset, slope
    0.704_f, -112.73_f, //root_offset, slope
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
  ExtCVConditioner<CV_PITCH, Average<4, 4>
                   > pitch_cv_ {calibration_data_.pitch_offset,
                                calibration_data_.pitch_slope, 
                                spi_adc_};
  ExtCVConditioner<CV_ROOT, Average<4, 2>
                   > root_cv_ {calibration_data_.root_offset,
                               calibration_data_.root_slope, 
                               spi_adc_};

  HysteresisFilter<1, 10> root_post_filter_;

  Parameters& params_;
  PolypticOscillator<block_size>& osc_;

  Sampler<f> pitch_cv_sampler_;

  uint8_t ext_cv_chan;
public:

  Control(Parameters& params, PolypticOscillator<block_size>& osc) :
    osc_(osc),
    params_(params) {}

  void ProcessSpiAdcInput() {
    if (ext_cv_chan) {
      pitch_cv_.Process();
      pitch_cv_.switch_channel();
    } else {
      root_cv_.Process();
      root_cv_.switch_channel();
    }
    ext_cv_chan = !ext_cv_chan;
  }

  void Poll(std::function<void(Event)> const& put) {

    // Process gates
    gates_.Debounce();

    if (gates_.freeze_.just_enabled()) {
      osc_.set_freeze(!osc_.frozen());
    } else if (gates_.freeze_.just_disabled()) {
      osc_.set_freeze(!osc_.frozen());
    }

    if (gates_.learn_.just_enabled() && osc_.learn_mode()) {
      put({NewNoteAfterDelay, 0});
    }

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
        mod *= 6.0_f;
      } else if (params_.modulation.mode == TWO) {
        mod *= 0.9_f;
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

      if (pitch_cv_.calibration_busy()) {
        auto cal_result = pitch_cv_.process_calibration();
        if (cal_result == CAL_STEP_RESULT_FAILURE)
          put({CalibrationFailed, 0});
        else if (cal_result == CAL_STEP_RESULT_SUCCESS)
          put({CalibrationStepDone, 0});
      }
      pitch_cv = pitch_cv_sampler_.Process(pitch_cv);
      pitch += pitch_cv;
      params_.pitch = pitch;

      params_.fine_tune =
        fine_tune > 0_f ? (fine_tune - 0.5_f) * kNewNoteFineRange : 0_f;
    }

    // ROOT
    { auto [root, new_note] = root_pot_.Process(put);
      root *= kRootPotRange;
      root += root_post_filter_.Process(root_cv_.last());

      if (root_cv_.calibration_busy()) {
        auto cal_result = root_cv_.process_calibration();
        if (cal_result == CAL_STEP_RESULT_FAILURE)
          put({CalibrationFailed, 0});
        else if (cal_result == CAL_STEP_RESULT_SUCCESS)
          put({CalibrationStepDone, 0});
      }

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
  void cache_all_alt_shift_pot_values() {
    spread_.pot_.cache();
    twist_.pot_.cache();
    warp_.pot_.cache();
    balance_.pot_.cache();
  }
  void cache_all_alt_learn_pot_values() {
    root_pot_.cache();
    pitch_pot_.cache();
  }

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


  CalibratorState calibration_state_;

  void calibration_reset() {
    calibration_state_ = NOT_CALIBRATING;
    pitch_cv_.reset_calibration();
    root_cv_.reset_calibration();
  }

  bool calibration_busy() { 
    return (pitch_cv_.calibration_busy() 
        || root_cv_.calibration_busy());
  }

  auto calibration_state() {
    return calibration_state_;
  }

  void next_calibration() {
    switch (calibration_state_) {
      case NOT_CALIBRATING: {
        calibration_state_ = CALIBRATING_PITCH_UNPATCHED;
        pitch_cv_.start_calibration(CALIBRATE_UNPATCHED);
      } break;

      case CALIBRATING_PITCH_UNPATCHED: {
        calibration_state_ = CALIBRATING_ROOT_UNPATCHED;
        root_cv_.start_calibration(CALIBRATE_UNPATCHED);
      } break;

      case CALIBRATING_ROOT_UNPATCHED: {
        calibration_state_ = CALIBRATING_PITCH_OFFSET;
        pitch_cv_.start_calibration(CALIBRATE_C2);
      } break;

      case CALIBRATING_PITCH_OFFSET: {
        calibration_state_ = CALIBRATING_PITCH_SLOPE;
        pitch_cv_.start_calibration(CALIBRATE_C4);
      } break;

      case CALIBRATING_PITCH_SLOPE: {
        calibration_state_ = CALIBRATING_ROOT_OFFSET;
        root_cv_.start_calibration(CALIBRATE_C2);
      } break;

      case CALIBRATING_ROOT_OFFSET: {
        calibration_state_ = CALIBRATING_ROOT_SLOPE;
        root_cv_.start_calibration(CALIBRATE_C4);
      } break;
    }
  }

  bool calibrate_offsets() {
    return warp_.cv_.calibrate_offset()
      && balance_.cv_.calibrate_offset()
      && twist_.cv_.calibrate_offset()
      && scale_.cv_.calibrate_offset()
      && modulation_.cv_.calibrate_offset()
      && spread_.cv_.calibrate_offset();
  }

  void SaveCalibration() {
    calibration_data_storage_.Save();
  }
};
