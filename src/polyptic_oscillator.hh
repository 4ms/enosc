#pragma once

#include <algorithm>

#include "dsp.hh"
#include "oscillator.hh"
#include "quantizer.hh"

class AmplitudeAccumulator {
  f amplitude = 1_f;
  f amplitudes = 0_f;
  f balance;
  f numOsc;
public:
  AmplitudeAccumulator(f t, f n) : balance(t), numOsc(n) {}
  f next() {
    f r =
      numOsc > 1_f ? amplitude :
      numOsc > 0_f ? amplitude * numOsc :
      0_f;
    amplitudes += r;
    amplitude *= balance;
    numOsc--;
    return r;
  }

  f sum() { return amplitudes; }
};

template<int block_size>
class PreListenOscillators : Nocopy {
  Oscillator oscs_[kMaxScaleSize];
  Buffer<u0_16, block_size> dummy_block_;
  OnePoleLp amp_lp_;

public:
  void Process(Parameters const &params, PreScale &scale,
               Buffer<f, block_size>& out1, Buffer<f, block_size>& out2) {
    out1.fill(0_f);
    out2.fill(0_f);

    f twist = params.twist.value;
    f warp = params.warp.value;
    f modulation = 0_f;    // no modulation in pre-listen

    f scale_size = amp_lp_.Process(0.05_f, f(scale.size()));
    AmplitudeAccumulator amplitudes { 1_f, scale_size };

    typename OscillatorPair<block_size>::processor_t process =
      OscillatorPair<block_size>::pick_processor(params.twist.mode, params.warp.mode);

    for (int i=0; i<kMaxScaleSize; ++i) {
      f freq = Freq::of_pitch(scale.get(i)).repr();
      (oscs_[i].*process)(freq, twist, warp,
                          modulation, 1_f, amplitudes.next(),
                          dummy_block_, dummy_block_, out1);
    }

    f sum = amplitudes.sum();
    f atten = 0.5_f / sum;
    atten *= 1_f + Data::normalization_factors.interpolate_from_index(sum);

    for (auto [o1, o2] : zip(out1, out2)) {
      o2 = o1 = o1 * atten;
    }
  }
};

template<int block_size>
class Oscillators : Nocopy {
  OscillatorPair<block_size> oscs_[kMaxNumOsc];
  Buffer<u0_16, block_size> modulation_blocks_[kMaxNumOsc+1];
  Buffer<u0_16, block_size> dummy_block_;
  bool frozen_ = false;
  bool temp_frozen_ = false;
  f lowest_pitch_;

  static inline bool pick_split(SplitMode mode, int i, int numOsc) {
    return
      mode == ALTERNATE ? !(i&1) :
      mode == LOW_HIGH ? i<numOsc/2 :
      i == 0;
  }

  inline std::tuple<Buffer<u0_16, block_size>&, Buffer<u0_16, block_size>&>
  pick_modulation_blocks(ModulationMode mode, int i, int numOsc) {
    if(mode == ONE) { //Up
      if (i==0) {
        return std::forward_as_tuple(dummy_block_, modulation_blocks_[0]);
      } else {
        return std::forward_as_tuple(modulation_blocks_[0], dummy_block_);
      }
    } else if (mode == TWO) { //All
      if (i==0) {
        return std::forward_as_tuple(dummy_block_, modulation_blocks_[i+1]);
      } else {
        return std::forward_as_tuple(modulation_blocks_[i], modulation_blocks_[i+1]);
      }
    } else { // mode == THREE //Down
      if (i==numOsc-1) {
        return std::forward_as_tuple(dummy_block_, modulation_blocks_[0]);
      } else {
        return std::forward_as_tuple(modulation_blocks_[0], dummy_block_);
      }
    }
  }

  class FrequencyAccumulator {
    f root;
    f pitch;
    f spread;
    f detune;
    f detune_accum = 0_f;
    Scale const &scale;
  public:
    FrequencyAccumulator(Scale const &g, f r, f p, f s, f d) :
      scale(g), root(r), pitch(p), spread(s), detune(d) {}

    f next_pitch() {
      PitchPair p = scale.Process(root);
      f lowest = p.crossfade < 0.5_f ? p.p1 : p.p2;
      return lowest + pitch + detune_accum;
    }

    FrequencyPair next() {

      // root > 0
      PitchPair p = scale.Process(root); // 2%

      p.p1 += pitch + detune_accum;
      p.p2 += pitch + detune_accum;

      f freq1 = Freq::of_pitch(p.p1).repr();
      f freq2 = Freq::of_pitch(p.p2).repr();

      root += spread;
      detune *= -1.2_f;
      detune_accum += detune;

      return {freq1, freq2, p.crossfade};
    }
  };

public:
  void Process(Parameters const &params, Scale const &scale,
               Buffer<f, block_size>& out1, Buffer<f, block_size>& out2) {
    out1.fill(0_f);
    out2.fill(0_f);

    int numOsc = params.alt.numOsc;

    AmplitudeAccumulator amplitude {params.balance, f(numOsc)};
    FrequencyAccumulator frequency {scale, params.root, params.pitch,
                                    params.spread, params.detune};

    lowest_pitch_ = frequency.next_pitch();

    TwistMode twist_mode = params.twist.mode;
    WarpMode warp_mode = params.warp.mode;
    f twist = params.twist.value;
    f warp = params.warp.value;
    f modulation = params.modulation.value;

    f crossfade_factor = params.alt.crossfade_factor;
    SplitMode stereo_mode = params.alt.stereo_mode;
    SplitMode freeze_mode = params.alt.freeze_mode;
    ModulationMode modulation_mode = params.modulation.mode;

    for (int i=0; i<kMaxNumOsc; ++i) {
      FrequencyPair p = frequency.next(); // 3%
      f amp = amplitude.next();
      Buffer<f, block_size>& out = pick_split(stereo_mode, i, numOsc) ? out1 : out2;
      auto [mod_in, mod_out] = pick_modulation_blocks(modulation_mode, i, numOsc);
      dummy_block_.fill(0._u0_16);
      bool frozen = (pick_split(freeze_mode, i, numOsc) && frozen_) || temp_frozen_;
      oscs_[i].Process(twist_mode, warp_mode,
                       p, frozen, crossfade_factor,
                       twist, warp, modulation, amp,
                       mod_in, mod_out, out);
    }

    temp_frozen_ = false;

    f atten1, atten2;

    f atten = 1_f / amplitude.sum();
    f balance = params.balance <= 1_f ? params.balance : 1_f / params.balance;
    atten *= 0.5_f + (0.5_f + Data::normalization_factors[numOsc]) * balance;

    atten1 = atten2 = atten;

    if (stereo_mode == LOWEST_REST) {
      // adjust attenuation to not clip the output
      atten1 = 0.7_f;
      atten2 = 0.6_f * atten;
    }

    for (auto [o1, o2] : zip(out1, out2)) {
      o1 *= atten1;
      o2 *= atten2;
    }
  }

  void set_freeze (bool frozen) { frozen_ = frozen; }
  void set_temporary_freeze() { temp_frozen_ = true; }
  bool frozen() { return frozen_; }
  f lowest_pitch() { return lowest_pitch_; }
};

template<int block_size>
class PolypticOscillator : public Oscillators<block_size>, PreListenOscillators<block_size> {
  Parameters& params_;
  Quantizer quantizer_;
  PreScale pre_scale_;
  Scale *current_scale_;

  bool pre_listen_ = false;
  bool follow_new_note_ = false;
  f manual_learn_offset_ = 0_f;

  int previous_scale_index;

public:
  PolypticOscillator(Parameters& params) : params_(params) {}

  void enable_pre_listen() { pre_listen_ = true; }
  void disable_pre_listen() { pre_listen_ = false; }
  void enable_follow_new_note() { follow_new_note_ = true; }
  void disable_follow_new_note() { follow_new_note_ = false; }

  void enable_learn() {
    pre_scale_.clear();
    manual_learn_offset_ = this->lowest_pitch() - params_.root;
  }

  bool disable_learn() {
    disable_pre_listen();
    bool wrap_octave = params_.scale.mode == OCTAVE;
    bool success = pre_scale_.copy_to(current_scale_, wrap_octave);
    if (success) quantizer_.Save();
    return success;
  }

  bool new_note(f x) {
    if (params_.scale.mode == TWELVE)
      x = x.integral();
    return pre_scale_.add(x);
  }

  bool empty_pre_scale() {
    return pre_scale_.size() == 0;
  }

  void change_last_note(f coarse, f fine) {
    if (params_.scale.mode == TWELVE)
      coarse = coarse.integral();
    pre_scale_.set_last(coarse + fine);
  }

  bool remove_last_note() {
    return pre_scale_.remove_last();
  }

  void reset_current_scale() {
    quantizer_.reset_scale(params_.scale);
    quantizer_.Save();
  }

  void Process(Buffer<Frame, block_size>& out) {
    Buffer<f, block_size> out1;
    Buffer<f, block_size> out2;

    if (pre_listen_) {
      if (follow_new_note_)
        change_last_note(params_.new_note + manual_learn_offset_, params_.fine_tune);
      PreListenOscillators<block_size>::Process(params_, pre_scale_, out1, out2);
    } else {
      if (previous_scale_index != params_.scale.value) {
        Oscillators<block_size>::set_temporary_freeze();
        previous_scale_index = params_.scale.value;
      }
      current_scale_ = quantizer_.get_scale(params_.scale);
      Oscillators<block_size>::Process(params_, *current_scale_, out1, out2);
    }

    for (auto [o1, o2, o] : zip(out1, out2, out)) {
      o.l = s9_23(o1);
      o.r = s9_23(o2);
    }
  }
};
