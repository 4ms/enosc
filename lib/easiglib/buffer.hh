#include <initializer_list>

#include "util.hh"
#include "numtypes.hh"

#pragma once

using index = u32;

template<class T, unsigned int SIZE>
struct Table {
  constexpr Table(std::initializer_list<T> data) :
    data_(data.begin()) {};
  constexpr T operator[](index idx) const { return this->data_[idx.repr()]; }

protected:
  const T *data_;
};

template<class T, unsigned int SIZE>
struct Buffer {
private:
  Table<T, SIZE> data_;
public:

  constexpr Buffer(std::initializer_list<T> data) : data_(data) {};

  constexpr index size() const {return index::of_long_long(SIZE);}
  constexpr T operator[](index idx) const { return this->data_[idx]; }

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
    s16 a = this->data_[integral];
    s16 b = this->data_[integral+1_u32];
    s1_15 frac = u0_16::narrow(p.fractional()).to_signed();
    return a + ((b-a) * frac).shiftr<16>();
  }
};


// TODO convert to numtypes
template<typename T, unsigned int SIZE>
class RingBuffer {
  T buffer_[SIZE] = {0};
  uint32_t cursor_ = SIZE;
public:
  void Write(T x) {
    buffer_[++cursor_ % SIZE] = x;
  }
  T Read(uint32_t n) {
    return buffer_[(cursor_ - n) % SIZE];
  }
  T ReadLast() {
    return buffer_[(cursor_+1) % SIZE];
  }
};

template<unsigned int SIZE>
struct RingBuffer<Float, SIZE> {
  void Write(Float x) {
    buffer_[++cursor_ % SIZE] = x;
  }
  Float Read(int n) {
    return buffer_[(cursor_ - n) % SIZE];
  }
  Float ReadLast() {
    return buffer_[(cursor_+1) % SIZE];
  }

  Float ReadLinear(Float x) {
    uint32_t index = static_cast<uint32_t>(x.repr());
    Float fractional = x - Float(index);
    Float x1 = buffer_[(cursor_ - index+1) % SIZE];
    Float x2 = buffer_[(cursor_ - index) % SIZE];
    return x1 + (x2 - x1) * fractional;
  }
private:
  Float buffer_[SIZE] = {0};
  uint32_t cursor_ = SIZE;
};
