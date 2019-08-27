#pragma once

#include <algorithm>

#include "dsp.hh"
#include "oscillator.hh"
#include "quantizer.hh"

template<int block_size>
class PreListenOscillators : Nocopy {
  Oscillator oscs_[kMaxGridSize];
  Buffer<u0_16, block_size> dummy_block_;

public:
  void Process(Parameters const &params, PreGrid &grid,
               Buffer<f, block_size>& out1, Buffer<f, block_size>& out2) {
    out1.fill(0_f);
    out2.fill(0_f);

    f twist = params.twist.value;
    f warp = params.warp.value;
    f modulation = 0_f;    // no modulation in pre-listen

    for (int i=0; i<grid.size(); ++i) {
      f freq = Freq::of_pitch(grid.get(i)).repr();
      Buffer<f, block_size>& out = i&1 ? out1 : out2; // alternate
      oscs_[i].Process<FEEDBACK, CRUSH>(freq, twist, warp,
                                        modulation, 1_f, 1_f,
                                        dummy_block_, dummy_block_, out);
    }

    f atten = 1_f / f(grid.size());
    atten *= 1_f + Data::normalization_factors[grid.size()];

    for (auto [o1, o2] : zip(out1, out2)) {
      o1 *= atten;
      o2 *= atten;
    }
  }
};

template<int block_size>
class Oscillators : Nocopy {
  OscillatorPair oscs_[kMaxNumOsc];
  Buffer<u0_16, block_size> modulation_blocks_[kMaxNumOsc+1];
  Buffer<u0_16, block_size> dummy_block_;
  bool frozen_ = false;

  static inline bool pick_split(SplitMode mode, int i, int numOsc) {
    return
      mode == ALTERNATE ? i&1 :
      mode == LOW_HIGH ? i<numOsc/2 :
      i == 0;
  }

  inline std::tuple<Buffer<u0_16, block_size>&, Buffer<u0_16, block_size>&>
  pick_modulation_blocks(ModulationMode mode, int i, int numOsc) {
    if(mode == ONE) {
      if (i==0) {
        return std::forward_as_tuple(dummy_block_, modulation_blocks_[i+1]);
      } else {
        return std::forward_as_tuple(modulation_blocks_[i], modulation_blocks_[i+1]);
      }
    } else if (mode == TWO) {
      if (i==0) {
        return std::forward_as_tuple(dummy_block_, modulation_blocks_[0]);
      } else {
        return std::forward_as_tuple(modulation_blocks_[0], dummy_block_);
      }
    } else { // mode == THREE
      if (i==numOsc-1) {
        return std::forward_as_tuple(dummy_block_, modulation_blocks_[0]);
      } else {
        return std::forward_as_tuple(modulation_blocks_[0], dummy_block_);
      }
    }
  }

  using processor_t = void (OscillatorPair::*)(FrequencyPair,
                                               bool, // frozen
                                               f,    // crossfade_factor
                                               f, // twist
                                               f, // warp
                                               f, // modulation
                                               f, // amplitude
                                               Buffer<u0_16, block_size>&, // mod_in
                                               Buffer<u0_16, block_size>&, // mod_out
                                               Buffer<f, block_size>&); // sum_output

  processor_t pick_processor(TwistMode t, WarpMode m) {
    static processor_t tab[3][3] = {
      &OscillatorPair::Process<FEEDBACK, FOLD, block_size>,
      &OscillatorPair::Process<FEEDBACK, CHEBY, block_size>,
      &OscillatorPair::Process<FEEDBACK, CRUSH, block_size>,
      &OscillatorPair::Process<PULSAR, FOLD, block_size>,
      &OscillatorPair::Process<PULSAR, CHEBY, block_size>,
      &OscillatorPair::Process<PULSAR, CRUSH, block_size>,
      &OscillatorPair::Process<DECIMATE, FOLD, block_size>,
      &OscillatorPair::Process<DECIMATE, CHEBY, block_size>,
      &OscillatorPair::Process<DECIMATE, CRUSH, block_size>,
    };

    return tab[t][m];
  }

  class AmplitudeAccumulator {
    f amplitude = 1_f;
    f amplitudes = 0_f;
    f tilt;
  public:
    AmplitudeAccumulator(f t) : tilt(t) {}
    f Next() {
      f r = amplitude;
      amplitudes += amplitude;
      amplitude *= tilt;
      return r;
    }
    f Sum() { return amplitudes; }
  };

  class FrequencyAccumulator {
    f root;
    f pitch;
    f spread;
    f detune;
    f detune_accum = 0_f;
    Grid const &grid;
  public:
    FrequencyAccumulator(Grid const &g, f r, f p, f s, f d) :
      grid(g), root(r), pitch(p), spread(s), detune(d) {}
    FrequencyPair Next() {

      // root > 0
      PitchPair p = grid.Process(root); // 2%

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
  void Process(Parameters const &params, Grid const &grid,
               Buffer<f, block_size>& out1, Buffer<f, block_size>& out2) {
    out1.fill(0_f);
    out2.fill(0_f);

    AmplitudeAccumulator amplitude {params.tilt};
    FrequencyAccumulator frequency {grid, params.root, params.pitch,
                                    params.spread, params.detune};

    processor_t process = pick_processor(params.twist.mode, params.warp.mode);

    f twist = params.twist.value;
    f warp = params.warp.value;
    f modulation = params.modulation.value;

    f crossfade_factor = params.crossfade_factor;
    int numOsc = params.numOsc;
    SplitMode stereo_mode = params.stereo_mode;
    SplitMode freeze_mode = params.freeze_mode;
    ModulationMode modulation_mode = params.modulation.mode;

    for (int i=0; i<numOsc; ++i) {
      FrequencyPair p = frequency.Next(); // 3%
      f amp = amplitude.Next();
      Buffer<f, block_size>& out = pick_split(stereo_mode, i, numOsc) ? out1 : out2;
      auto [mod_in, mod_out] = pick_modulation_blocks(modulation_mode, i, numOsc);
      dummy_block_.fill(0._u0_16);
      bool frozen = pick_split(freeze_mode, i, numOsc) && frozen_;
      (oscs_[i].*process)(p, frozen, crossfade_factor, twist, warp, modulation, amp,
                         mod_in, mod_out, out);
    }

    f atten = 1_f / amplitude.Sum();
    f tilt = params.tilt <= 1_f ? params.tilt : 1_f / params.tilt;
    atten *= 0.5_f + (0.5_f + Data::normalization_factors[numOsc]) * tilt;

    f atten2 = stereo_mode == LOWEST_REST ? atten * 0.5_f : atten;

    for (auto [o1, o2] : zip(out1, out2)) {
      o1 *= atten;
      o2 *= atten2;
    }
  }

  void set_freeze (bool frozen) { frozen_ = frozen; }
  bool frozen() { return frozen_; }
};

template<int block_size>
class PolypticOscillator : public Oscillators<block_size>, PreListenOscillators<block_size> {
  Parameters& params_;
  Quantizer quantizer_;
  PreGrid pre_grid_;
  Grid *current_grid_;

  bool pre_listen_ = false;
  bool follow_new_note_ = false;

public:
  PolypticOscillator(Parameters& params) : params_(params) {}

  void enable_pre_listen() { pre_listen_ = true; }
  void disable_pre_listen() { pre_listen_ = false; }
  void enable_follow_new_note() { follow_new_note_ = true; }
  void disable_follow_new_note() { follow_new_note_ = false; }

  void enable_learn() {
    pre_grid_.clear();
  }

  bool disable_learn() {
    disable_pre_listen();
    bool wrap_octave = params_.grid.mode == OCTAVE;
    return pre_grid_.copy_to(current_grid_, wrap_octave);
  }

  bool new_note(f x) {
    if (params_.grid.mode == TWELVE)
      x = x.integral();
    return pre_grid_.add(x);
  }

  void change_last_note(f coarse, f fine) {
    if (params_.grid.mode == TWELVE)
      coarse = coarse.integral();
    pre_grid_.set_last(coarse + fine);
  }

  bool remove_last_note() {
    return pre_grid_.remove_last();
  }

  void Process(Buffer<Frame, block_size>& out) {
    Buffer<f, block_size> out1;
    Buffer<f, block_size> out2;

    if (pre_listen_) {
      if (follow_new_note_)
        change_last_note(params_.new_note, params_.fine_tune);
      PreListenOscillators<block_size>::Process(params_, pre_grid_, out1, out2);
    } else {
      current_grid_ = quantizer_.get_grid(params_.grid);
      Oscillators<block_size>::Process(params_, *current_grid_, out1, out2);
    }

    for (auto [o1, o2, o] : zip(out1, out2, out)) {
      o.l = s1_15(o1);
      o.r = s1_15(o2);
    }
  }
};
