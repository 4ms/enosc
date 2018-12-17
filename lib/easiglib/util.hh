#pragma once

#include <algorithm>

template<int N>
struct Log2 {
  static constexpr int val = Log2<N/2>::val + 1;
};

template<>
struct Log2<1> { static constexpr int val = 0; };

constexpr bool is_power_of_2(int v) {
    return v && ((v & (v - 1)) == 0);
}

constexpr int ipow(int a, int b) {
  return b==0 ? 1 : a * ipow(a, b-1);
}

class Nocopy {
public:
  Nocopy(const Nocopy&) = delete;
  Nocopy& operator=(const Nocopy&) = delete;
protected:
  constexpr Nocopy() = default;
  ~Nocopy() = default;
};

template<class T>
struct Block {
  Block(T* data, int size) : data_(data), size_(size) {}
  T& operator [] (unsigned int index) {
    return data_[index];
  }
  T const& operator [] (unsigned int index) const {
    return data_[index];
  }

  void fill(T x) { std::fill(data_, data_+size_, x); }
  T* begin() { return data_; }
  T* end() { return data_ + size_; }

  int size() {return size_; }
private:
  T *data_;
  int size_;
};
