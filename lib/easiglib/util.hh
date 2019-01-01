#pragma once

#include <algorithm>
#include <functional>

template<class T, T x, T y>
struct Max {
  static constexpr T val = x>y ? x : y;
};

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

template <class T, template<class> class crtpType>
struct crtp {
  T& operator*() { return static_cast<T&>(*this); }
  T const& operator*() const { return static_cast<T const&>(*this); }
private:
  crtp(){}
  friend crtpType<T>;
};

// Observer pattern
template<typename ... DATA>
struct Subject {
  void notify(DATA ... args) { observer_(args ...); }
  Subject(std::function<void(DATA ...)> observer) : observer_(observer) {}
private:
  std::function<void(DATA ...)> observer_;
};
