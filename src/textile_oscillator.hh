#pragma once

#include "numtypes.hh"
#include "dsp.hh"
#include "data.hh"

constexpr const int kBufferSize = 256;

class DrunkWalker {
  index delay_ = 0_u32;
public:
  index Process(f window_scale, f window_center) {
    f t = window_center - 0.5_f;
    f r = (Random::Float01() - 0.5_f + t);
    r *= window_scale * 10_f;
    if (r > 0_f) {
      if (delay_ + index(r) >= index::of_repr(kBufferSize)) {
        delay_ = index::of_repr(kBufferSize-1);
      } else {
        delay_ += index(r);
      }
    } else {
      if (delay_ > index(-r))
        delay_ -= index(-r);
      else {
        delay_ = 0_u32;
      }
    }
    return delay_;
  }
};

class Ornamenter {
public:
  index Process(f proba) {
    f r2 = Random::Float01();
    r2 *= r2; r2 *= r2; r2 *= r2; r2 *= r2;
    index ornament = 0_u32;
    if (r2 <= proba) {
      ornament = u32(f(Random::Word() % 4) * proba);
    }
    return ornament;
  }
};

class TimeDivider {
  int i=0, j=0;
public:
  index Process(int divider, int window_size) {
    int r = i-j;
    i++;
    if (i % divider == 0) {j++;}
    if (i >= window_size * divider) {i=0; j=0;}
    return index::of_repr(r);
  }
};

class PitchProcessor {
  unsigned int div;
  f pitches_[kNumOsc];
  bool has_changed_[kNumOsc];
  TimeDivider time_dividers_[kNumOsc];
  DrunkWalker drunk_walkers_[kNumOsc];
  Ornamenter ornamenters_[kNumOsc];
  RingBuffer<Float, kBufferSize> pitch_buffer_;
public:

  PitchProcessor() {
    std::fill(pitches_, pitches_+kNumOsc, 69_f);
  }

  void Process(const Parameters &params) {
    pitch_buffer_.Write(params.pitch + params.pitch_offset);
    f delay_spread = params.delay * params.delay * 12_f;
    f delay = 0_f;
    f division_spread = params.division * 8_f / f(kNumOsc);
    f division = 1_f;
    f transposition_spread = -params.transposition * 8_f;
    f transposition = transposition_spread * f(kNumOsc) * -0.6_f;

    for (int i=0; i<kNumOsc; i++) {
      f old_pitch = pitches_[i];

      index idx = index(delay);
      idx += drunk_walkers_[i].Process(params.proba_window_scale,
                                       params.drunk_delay);
      idx += ornamenters_[i].Process(params.ornament_proba);

      div = index(division).repr();
      if (params.division_mode == ODD) {
        div = (div - 1) / 2 * 2 + 1;
      } else if (params.division_mode == POW_OF_TWO) {
        div--;
        div |= div >> 1;
        div |= div >> 2;
        div |= div >> 4;
        div |= div >> 8;
        div |= div >> 16;
        div++;
      }

      idx += time_dividers_[i].Process(div, params.division_window);

      f trans_step =
        params.transposition_mode == THREE_ST ? 10_f :
        params.transposition_mode == OCTAVE ? 12_f :
        1_f; // CHROMATIC

      f trans = (transposition / trans_step).integral() * trans_step;

      pitches_[i] = pitch_buffer_.Read(idx) + trans;

      // small detune
      pitches_[i] += f(i).sqrt() * 0.005_f;

      has_changed_[i] = (pitches_[i] - old_pitch).abs() > 0.1_f;

      delay += delay_spread;
      division += division_spread;
      transposition += transposition_spread;
    }
  }

  f get(int i) {
    return pitches_[i];
  }

  bool has_changed(int i) {
    return has_changed_[i];
  }
};

class TimbreProcessor {
  f timbres_[kNumOsc];
public:
  void Process(const Parameters &params) {
    for (int i=0; i<kNumOsc; i++) {
      f t = params.timbre - 0.5_f;
      f r = (Random::Float01() - 0.5_f + t) * 0.1_f;
      r *= params.proba_window_scale;
      timbres_[i] += r;
      timbres_[i] = timbres_[i].clip(0_f, 1_f);
    }
  }

  f get(int i) {
    return timbres_[i];
  }
};

class TOscillator {
  u0_32 phase_ = u0_32::of_repr(Random::Word());
  s1_15 history_ = 0._s1_15;
  IOnePoleLp<s1_15, 2> lp_;
  IOnePoleLp<u0_32, 3> lp_freq_;
  IOnePoleLp<u0_16, 5> lp_fb_;
public:

  s1_15 Process(u0_32 freq, u0_16 feedback) {
    lp_freq_.Process(freq, &freq);
    lp_fb_.Process(feedback, &feedback);
    s1_31 fb = history_ * feedback.to_signed();
    u0_32 phase = phase_;
    phase += freq + fb.to_unsigned() + u0_32(feedback);
    s1_15 sample = Data::short_sine.interpolate(phase).movl<15>();
    phase_ += freq;
    lp_.Process(sample, &history_);
    return sample;
  }
};

class ADEnvelope {
  f phase_;
  enum state {ATTACK, DECAY} state_ = ATTACK;
public:
  void trigger() {
    state_ = ATTACK;
    // phase_ = 0_f;
  }

  f Process(f attack, f decay) {
    if (state_ == ATTACK) {
      phase_ += attack;
      if (phase_ > 1_f) {
        phase_ = 1_f;
        state_ = DECAY;
      }
    } else {
      phase_ -= decay;
      if (phase_ < 0_f) phase_ = 0_f;
    }
    return phase_;
  }
};

class TOscillators {
  TOscillator osc_[kNumOsc];
  ADEnvelope env_[kNumOsc];
  PitchProcessor pitch_proc_;
  TimbreProcessor timbre_proc_;
public:
  void Process(const Parameters &params, s1_15 *output, size_t size) {
    s17_15 out[size];
    std::fill(out, out+size, 0._s17_15);

    f poly = 1_f + params.polyphony * f(kNumOsc-1);

    for(int i=0; i<kNumOsc; i++) {
      Freq freq = Freq(pitch_proc_.get(i));
      f amplitude = poly > 1_f ? 1_f : poly < 0_f ? 0_f : poly;
      amplitude *= amplitude * amplitude;

      // antialiasing
      f aliasing_factor = freq.repr();
      if (aliasing_factor > 0.5_f) {
        amplitude = 0_f;
      } else if (aliasing_factor > 0.25_f) {
        amplitude *= 2_f - 4_f * aliasing_factor;
      }

      f timbre = timbre_proc_.get(i);
      u0_16 fb = u0_16::inclusive(timbre);

      for (size_t s = 0; s<size; s++) {
        s1_15 x = osc_[i].Process(freq.to_increment(), fb);
        f envelope = env_[i].Process(params.attack * freq.repr(), params.decay * freq.repr());
        envelope *= envelope;
        x *= s1_15::inclusive(amplitude);
        x *= s1_15::inclusive(envelope);
        out[s] += s17_15(x);
      }
      poly--;
    }

    for (size_t s = 0; s<size; s++) {
      f normalized = f(out[s]) * Data::normalization_factors[kNumOsc-1];
      normalized = Math::softclip1(normalized * 0.5_f);
      output[s] = s17_15(normalized).template to_sat<1,15>();
    }
  }

  void clock_tick(Parameters& params) {
    pitch_proc_.Process(params);
    timbre_proc_.Process(params);
    for (int i=0; i<kNumOsc; i++) {
      if (pitch_proc_.has_changed(i))
        env_[i].trigger();
    }
  }
};


class TextileOscillator {
  TOscillators osc_;
public:
  void Process(Parameters &params, Frame *out, int size) {
    s1_15 buffer[size];
    osc_.Process(params, buffer, size);

    s1_15 *b = buffer;
    for(s1_15 *b = buffer; b < buffer+size; b++) {
      out->l = *b;
      out->r = *b;
      out++;
    }
  }
};
