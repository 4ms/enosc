#pragma once

#include <algorithm>

#include "dsp.hh"
#include "oscillator.hh"
#include "quantizer.hh"

template<int block_size>
class Oscillators : Nocopy {
  OscillatorPair oscs_[kMaxNumOsc];
  u0_16 modulation_blocks_[kMaxNumOsc+1][block_size];
  u0_16 dummy_block_[block_size];

  static inline bool pick_output(StereoMode mode, int i, int numOsc) {
    return
      mode == ALTERNATE ? i&1 :
      mode == SPLIT ? i<numOsc/2 :
      i == 0;
  }

  inline std::pair<u0_16*, u0_16*>
  pick_modulation_blocks(ModulationMode mode, int i, int numOsc) {
    if(mode == ONE) {
      if (i==0) {
        return std::make_pair(dummy_block_, modulation_blocks_[i+1]);
      } else {
        return std::make_pair(modulation_blocks_[i], modulation_blocks_[i+1]);
      }
    } else if (mode == TWO) {
      if (i==0) {
        return std::make_pair(dummy_block_, modulation_blocks_[0]);
      } else {
        return std::make_pair(modulation_blocks_[0], dummy_block_);
      }
    } else { // mode == THREE
      if (i==numOsc-1) {
        return std::make_pair(dummy_block_, modulation_blocks_[0]);
      } else {
        return std::make_pair(modulation_blocks_[0], dummy_block_);
      }
    }
  }

  using processor_t = void (OscillatorPair::*)(FrequencyPair, f, f, f, f,
                                               Block<u0_16, block_size>, Block<u0_16, block_size>, Block<f, block_size>);

  processor_t pick_processor(TwistMode t, WarpMode m) {
    static processor_t tab[3][3] = {
      &OscillatorPair::Process<FEEDBACK, CRUSH, block_size>,
      &OscillatorPair::Process<FEEDBACK, CHEBY, block_size>,
      &OscillatorPair::Process<FEEDBACK, FOLD, block_size>,
      &OscillatorPair::Process<PULSAR, CRUSH, block_size>,
      &OscillatorPair::Process<PULSAR, CHEBY, block_size>,
      &OscillatorPair::Process<PULSAR, FOLD, block_size>,
      &OscillatorPair::Process<DECIMATE, CRUSH, block_size>,
      &OscillatorPair::Process<DECIMATE, CHEBY, block_size>,
      &OscillatorPair::Process<DECIMATE, FOLD, block_size>,
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

      PitchPair p = grid.Process(root); // 2%

      p.p1 += pitch + detune_accum;
      p.p2 += pitch + detune_accum;

      f freq1 = Freq::of_pitch(p.p1).repr();
      f freq2 = Freq::of_pitch(p.p2).repr();

      root += spread;
      detune_accum += detune;

      return {freq1, freq2, p.crossfade};
    }
  };

public:
  void Process(Parameters const &params, Grid const &grid,
               Block<f, block_size> out1, Block<f, block_size> out2) {
    out1.fill(0_f);
    out2.fill(0_f);

    AmplitudeAccumulator amplitude {params.tilt};
    FrequencyAccumulator frequency {grid, params.root, params.pitch,
                                    params.spread, params.detune};

    processor_t process = pick_processor(params.twist.mode, params.warp.mode);
    f twist = params.twist.value;
    f warp = params.warp.value;
    f modulation = params.modulation.value;
    int numOsc = params.numOsc;
    StereoMode stereo_mode = params.stereo_mode;
    ModulationMode modulation_mode = params.modulation.mode;

    for (int i=0; i<numOsc; ++i) {
      FrequencyPair p = frequency.Next(); // 3%
      f amp = amplitude.Next();
      Block<f, block_size> out =
        pick_output(stereo_mode, i, numOsc) ? out1 : out2;
      std::pair<u0_16*, u0_16*> mod_blocks =
        pick_modulation_blocks(modulation_mode, i, numOsc);
      Block<u0_16, block_size> mod_in(mod_blocks.first);
      Block<u0_16, block_size> mod_out(mod_blocks.second);
      // TODO cleanup: avoid manipulating bare pointers to buffers
      // need to declare the Blocks globally, ie. with an allocating constructor
      std::fill(dummy_block_, dummy_block_+block_size, 0._u0_16);
      (oscs_[i].*process)(p, twist, warp, amp, modulation,
                         mod_in, mod_out, out);
    }

    f atten = 1_f / amplitude.Sum();

    for (auto o : zip(out1, out2)) {
      f& o1 = get<0>(o);
      f& o2 = get<1>(o);
      o1 *= atten;
      o2 *= atten;
    }
  }

  void set_freeze(int voiceNr) {
    oscs_[voiceNr].set_freeze(true);
  }

  void unfreeze_all() {
    for (auto& o : oscs_) o.set_freeze(false);
  }
};

template<int block_size>
class PolypticOscillator : Oscillators<block_size> {
  using Base = Oscillators<block_size>;
  Parameters& params_;
  Quantizer quantizer_;
  PreGrid pre_grid_;
  Grid *current_grid_ = quantizer_.get_grid(0);

  bool learn_mode = false;
public:
  PolypticOscillator(
    Parameters& params,
    std::function<void(bool)> onNewNote,
    std::function<void(bool)> onExitLearn)
    : params_(params), onNewNote_(onNewNote), onExitLearn_(onExitLearn) {}

  Subject<bool> onNewNote_;
  Subject<bool> onExitLearn_;

  bool learn_enabled() { return learn_mode; }
  void enable_learn() {
    pre_grid_.clear();
    learn_mode = true;
  }
  void disable_learn() {
    if (pre_grid_.size() >= 2) {
      pre_grid_.copy_to(current_grid_);
      onExitLearn_.notify(true);
    } else {
      onExitLearn_.notify(false);
    }
    learn_mode = false;
  }

  void new_note(f x) {
    if (learn_mode) {
      bool success = pre_grid_.add(x);
      onNewNote_.notify(success);
    }
  }

  void freeze_selected_osc() { Base::set_freeze(params_.selected_osc); }
  void unfreeze_all() { Base::unfreeze_all(); }

  void Process(Block<Frame, block_size> out) {
    f buffer[2][block_size];
    Block<f, block_size> out1 {buffer[0]};
    Block<f, block_size> out2 {buffer[1]};

    current_grid_ = quantizer_.get_grid(params_.grid);

    Base::Process(params_, *current_grid_, out1, out2);

    for (auto x : zip(out1, out2, out)) {
      f &o1 = std::get<0>(x);
      f &o2 = std::get<1>(x);
      Frame &o = std::get<2>(x);
      o.l = s1_15(o1);
      o.r = s1_15(o2);
    }
  }
};
