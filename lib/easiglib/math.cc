#include "math.hh"

Math::Math() {

  float x=1.0f;
  for (int i=0; i<exp2_size; i++) {
    exp2_table[i] = static_cast<uint32_t>(x * (1<<23));
    x *= exp2_increment;
  }
}

uint32_t Math::exp2_table[exp2_size];
