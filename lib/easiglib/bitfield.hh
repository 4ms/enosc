#include "numtypes.hh"

template<int WIDTH>
struct Bitfield {
  using Base = typename Basetype<WIDTH, UNSIGNED>::T;

  explicit Bitfield(Base val) : val_(val) {}
  Base repr() { return val_; }

  bool is_set(int i) {
    return (val_ & (1 << i)) != 0;
  }
  Bitfield set(int i) {
    return Bitfield(val_ | (1U << i));
  }

  Bitfield reset(int i) {
    return Bitfield(val_ & ~(1U << i));
  }

  int set_bits() {
    int n=val_, count=0;
    while (n) {
      n &= (n-1);
      count++;
    }
    return count; 
  }
  

private:
  Base val_ = 0;
};
