

template<int N>
struct Log2 {
  static constexpr int val = Log2<N/2>::val + 1;
};

template<>
struct Log2<1> { static constexpr int val = 0; };

constexpr bool is_power_of_2(int v) {
    return v && ((v & (v - 1)) == 0);
}
