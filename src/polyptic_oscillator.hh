#pragma once

#include <algorithm>

#include "dsp.hh"
#include "oscillator.hh"
#include "quantizer.hh"

class Oscillators : Nocopy {
  DoubleOscillator osc_[kNumOsc];

  static bool pick_output(StereoMode mode, int i) {
    return
      mode == ALTERNATE ? i&1 :
      mode == SPLIT ? i<=kNumOsc/2 :
      i == 0;
  }

  using processor_t = void (DoubleOscillator::*)(f, f, f, f, f, f, Block<f>);

  processor_t choose_processor(TwistMode t, WarpMode m) {
    return
      t == FEEDBACK && m == CRUSH ? &DoubleOscillator::Process<FEEDBACK, CRUSH> :
      t == FEEDBACK && m == CHEBY ? &DoubleOscillator::Process<FEEDBACK, CHEBY> :
      t == FEEDBACK && m == FOLD ? &DoubleOscillator::Process<FEEDBACK, FOLD> :
      t == PULSAR && m == CRUSH ? &DoubleOscillator::Process<PULSAR, CRUSH> :
      t == PULSAR && m == CHEBY ? &DoubleOscillator::Process<PULSAR, CHEBY> :
      t == PULSAR && m == FOLD ? &DoubleOscillator::Process<PULSAR, FOLD> :
      t == DECIMATE && m == CRUSH ? &DoubleOscillator::Process<DECIMATE, CRUSH> :
      t == DECIMATE && m == CHEBY ? &DoubleOscillator::Process<DECIMATE, CHEBY> :
      t == DECIMATE && m == FOLD ? &DoubleOscillator::Process<DECIMATE, FOLD> :
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
    Grid grid;
  public:
    FrequencyAccumulator(Grid &g, f r, f p, f s, f d) :
      grid(g), root(r), pitch(p), spread(s), detune(d) {}
    void Next(f& freq1, f& freq2, f& phase) {

      f p1, p2;
      grid.Process(root, p1, p2, phase);

      p1 += pitch + detune_accum;
      p2 += pitch + detune_accum;

      freq1 = Freq::of_pitch(p1).repr();
      freq2 = Freq::of_pitch(p2).repr();

      root += spread;
      detune_accum += detune;
    }
  };

public:
  void Process(Parameters const &params, Grid grid,
               Block<f> out1, Block<f> out2) {
    out1.fill(0_f);
    out2.fill(0_f);

    AmplitudeAccumulator amplitude {params.tilt};
    FrequencyAccumulator frequency {grid, params.root, params.pitch,
                                    params.spread, params.detune};

    processor_t process = choose_processor(params.twist.mode, params.warp.mode);
    f twist = params.twist.value;
    f warp = params.warp.value;

    for (int i=0; i<kNumOsc; i++) {
      // antialias
      f freq1, freq2, phase;
      frequency.Next(freq1, freq2, phase);

      f amp = amplitude.Next();
      Block<f> out = pick_output(params.stereo_mode, i) ? out1 : out2;
      (osc_[i].*process)(freq1, freq2, phase, twist, warp, amp, out);
    }

    f atten = 1_f / amplitude.Sum();

    f *begin1 = out1.begin();
    for (f& o2 : out2) {
      f& o1 = *begin1;
      o1 *= atten;
      o2 *= atten;
      begin1++;
    }
  }
};

class PolypticOscillator : Nocopy {
  Oscillators oscs_;
  Quantizer quantizer_;
  PreGrid pre_grid_;

  bool learn_mode = false;
public:
  bool learn_enabled() { return learn_mode; }
  void enable_learn() {
    pre_grid_.clear();
    learn_mode = true;
  }
  void disable_learn() {
    pre_grid_.copy_to(quantizer_.get_grid(0)); // TODO
    learn_mode = true;
  }

  void new_note(f x) {
    if (learn_mode) {
      // TODO exploit function return for display
      pre_grid_.add(x);
    }
  }

  void Process(Parameters const &params, Block<Frame> out) {
    f buffer[2][out.size()];
    Block<f> out1 {buffer[0], out.size()};
    Block<f> out2 {buffer[1], out.size()};

    Grid grid = quantizer_.get_grid(params.grid);

    oscs_.Process(params, grid, out1, out2);

    f *b1 = out1.begin();
    f *b2 = out2.begin();
    for (Frame& o : out) {
      o.l = s1_15(*b1);
      o.r = s1_15(*b2);
      b1++; b2++;
    }
  }
};
