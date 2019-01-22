#pragma once

#include <algorithm>

#include "dsp.hh"
#include "oscillator.hh"
#include "quantizer.hh"

template<int size>
class Oscillators : Nocopy {
  OscillatorPair oscs_[kNumOsc];
  u0_16 modulation_blocks_[kNumOsc+1][size];
  u0_16 dummy_block_[size];

  static inline bool pick_output(StereoMode mode, int i) {
    return
      mode == ALTERNATE ? i&1 :
      mode == SPLIT ? i<=kNumOsc/2 :
      i == 0;
  }

  inline std::pair<u0_16*, u0_16*>
  pick_modulation_blocks(ModulationMode mode, int i) {
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
      if (i&1) {
        return std::make_pair(dummy_block_, modulation_blocks_[i]);
      } else {
        return std::make_pair(modulation_blocks_[i+1], dummy_block_);
      }
    }
  }

  using processor_t = void (OscillatorPair::*)(FrequencyPair, f, f, f, f,
                                               Block<u0_16, size>, Block<u0_16, size>, Block<f, size>);

  processor_t choose_processor(TwistMode t, WarpMode m) {
    return
      t == FEEDBACK && m == CRUSH ? &OscillatorPair::Process<FEEDBACK, CRUSH, size> :
      t == FEEDBACK && m == CHEBY ? &OscillatorPair::Process<FEEDBACK, CHEBY, size> :
      t == FEEDBACK && m == FOLD ? &OscillatorPair::Process<FEEDBACK, FOLD, size> :
      t == PULSAR && m == CRUSH ? &OscillatorPair::Process<PULSAR, CRUSH, size> :
      t == PULSAR && m == CHEBY ? &OscillatorPair::Process<PULSAR, CHEBY, size> :
      t == PULSAR && m == FOLD ? &OscillatorPair::Process<PULSAR, FOLD, size> :
      t == DECIMATE && m == CRUSH ? &OscillatorPair::Process<DECIMATE, CRUSH, size> :
      t == DECIMATE && m == CHEBY ? &OscillatorPair::Process<DECIMATE, CHEBY, size> :
      t == DECIMATE && m == FOLD ? &OscillatorPair::Process<DECIMATE, FOLD, size> :
      NULL;
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

      PitchPair p = grid.Process(root);

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
               Block<f, size> out1, Block<f, size> out2) {
    out1.fill(0_f);
    out2.fill(0_f);

    AmplitudeAccumulator amplitude {params.tilt};
    FrequencyAccumulator frequency {grid, params.root, params.pitch,
                                    params.spread, params.detune};

    processor_t process = choose_processor(params.twist.mode, params.warp.mode);
    f twist = params.twist.value;
    f warp = params.warp.value;
    f modulation = params.modulation.value;

    for (int i=0; i<kNumOsc; ++i) {
      FrequencyPair p = frequency.Next();
      f amp = amplitude.Next();
      Block<f, size> out = pick_output(params.stereo_mode, i) ? out1 : out2;
      std::pair<u0_16*, u0_16*> mod_blocks = pick_modulation_blocks(params.modulation.mode, i);
      Block<u0_16, size> mod_in(mod_blocks.first);
      Block<u0_16, size> mod_out(mod_blocks.second);
      std::fill(dummy_block_, dummy_block_+size, 0._u0_16);
      (oscs_[i].*process)(p, twist, warp, amp, modulation,
                         mod_in, mod_out, out);
    }

    f atten = 1_f / amplitude.Sum();

    // TODO double iteration on out1 and out2
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

template<int size>
class PolypticOscillator : Oscillators<size> {
  using Base = Oscillators<size>;
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

  void Process(Block<Frame, size> out) {
    f buffer[2][size];
    TripleBlock<f, f, Frame, size> block {buffer[0], buffer[1], out.begin()};

    current_grid_ = quantizer_.get_grid(params_.grid);

    Base::Process(params_, *current_grid_, block.first(), block.second());

    for (auto x : block) {
      f &o1 = std::get<0>(x);
      f &o2 = std::get<1>(x);
      Frame &o = std::get<2>(x);
      o.l = s1_15(o1);
      o.r = s1_15(o2);
    }
  }
};
