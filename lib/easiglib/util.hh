#pragma once

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
