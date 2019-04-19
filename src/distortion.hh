#pragma once

#include "dsp.hh"

namespace Distortion {

  template<WarpMode> inline f warp(s1_15, f);
  template<TwistMode> inline u0_32 twist(u0_32, f);

  template<>
  inline u0_32 twist<PULSAR>(u0_32 phase, f amount) {
    // amount: 0..255
    u8_8 p = u8_8(amount);
    return u0_32::wrap((u0_16::narrow(phase) * p).clip());
  }

  template<>
  inline u0_32 twist<DECIMATE>(u0_32 phase, f amount) {
    u0_16 am = u0_16(amount);
    uint32_t x = phase.repr();
    x ^= (u0_16::narrow(phase)*am).repr();
    x ^= (0.25_u0_16 * am).repr();
    return u0_32::of_repr(x);
  }

  template<>
  inline u0_32 twist<FEEDBACK>(u0_32 phase, f amount) {
    return phase;
  }

  // template<>
  // inline f warp<CRUSH>(s1_15 sample, f amount) {
  //   f x = sample.to_float();
  //   union { f a; uint32_t b; } t = {x};
  //   t.b ^= (uint32_t)(((1 << 23)-1) * amount.repr());
  //   return t.a;
  // }

  template<>
  inline f warp<CRUSH>(s1_15 sample, f amount) {
    f x = sample.to_float();
    amount *= 7_f;
    int bits = amount.floor() + 16;
    f frac = amount.fractional();
    int b = (bits + (x.abs()<frac ? 1 : 0));
    union { f a; uint32_t b; } t = {x};
    t.b ^= (1 << b) - 1;
    return t.a;
  }

  template<>
  inline f warp<CHEBY>(s1_15 x, f amount) {
    amount *= (Data::cheby.size() - 1_u32).to_float();
    index idx = index(amount);
    f frac = amount.fractional();
    u0_32 phase = u0_32(x.to_unsigned_scale());
    f s1 = Data::cheby[idx].interpolate(phase);
    f s2 = Data::cheby[idx+1_u32].interpolate(phase);
    return Signal::crossfade(s1, s2, frac);
  }

  template<>
  inline f warp<FOLD>(s1_15 x, f amount) {
    u0_16 sample = x.abs().to_unsigned();
    u0_32 phase = sample * u0_16(amount);
    f res = Data::fold.interpolate(phase);
    return x > 0._s1_15 ? res : -res;
  }
};
