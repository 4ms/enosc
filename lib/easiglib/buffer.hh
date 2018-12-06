#include <initializer_list>

#include "util.hh"
#include "numtypes.hh"

#pragma once

using index = u32;

template<class T>
struct Table {
  constexpr Table(std::initializer_list<T> data) :
    data_(data.begin()) {};
  constexpr T operator[](index idx) const { return this->data_[idx.repr()]; }
  constexpr T operator[](size_t idx) const { return this->data_[idx]; }

protected:
  const T *data_;
};

template<class T, unsigned int SIZE>
struct Buffer {
private:
  Table<T> data_;
public:

  constexpr Buffer(std::initializer_list<T> data) : data_(data) {};

  constexpr index size() const {return index::of_long_long(SIZE);}
  constexpr T operator[](index idx) const { return this->data_[idx]; }
  constexpr T operator[](size_t idx) const { return this->data_[idx]; }

  // zero-order hold
  constexpr T operator[](f const phase) const {
    phase *= (size()-1_u32).to_float();
    index integral = index(phase);
    return this->data_[integral];;
  }

  // zero-order hold
  constexpr T operator[](u0_32 const phase) const {
    static_assert(is_power_of_2(SIZE-1), "only power-of-two-sized buffers");
    constexpr int BITS = Log2<SIZE>::val;
    index i = phase.movr<BITS>().integral();
    return this->data_[i];
  }

  constexpr T interpolate(f phase) const {
    phase *= (size()-1_u32).to_float();
    index integral = index(phase);
    f fractional = phase - integral.to_float();
    T a = this->data_[integral];
    T b = this->data_[integral+1_u32];
    return a + (b - a) * fractional;
  }

  constexpr T interpolate(u0_32 const phase) const {
    static_assert(is_power_of_2(SIZE-1), "only power-of-two-sized buffers");
    constexpr int BITS = Log2<SIZE>::val;
    Fixed<UNSIGNED, BITS, 32-BITS> p = phase.movr<BITS>();
    u32_0 integral = p.integral();
    T a = this->data_[integral];
    T b = this->data_[integral+1_u32];
    s1_15 frac = u0_16::narrow(p.fractional()).to_signed();
    return a + ((b-a) * frac).template shiftr<16>();
  }
};

template<typename T, unsigned int SIZE>
class RingBuffer {
  T buffer_[SIZE];
  static constexpr index S = index::of_repr(SIZE);
  index cursor_ = S;
public:
  index size() { return S; }
  void Write(T x) {
    ++cursor_;
    index idx = cursor_ % S;
    buffer_[(idx).repr()] = x;
  }
  T Read(index n) {
    // TODO specialized version when S is 2^n
    return buffer_[((cursor_ - n) % S).repr()];
  }
  T ReadLast() {
    return buffer_[((cursor_+1_u32) % S).repr()];
  }
};

template<typename T>
class RingBuffer<T, 1> {
  T buffer_;
public:
  void Write(T x) { buffer_ = x; }
  void Read(index n) { return buffer_; }
  void ReadLast() { return buffer_; }
};
