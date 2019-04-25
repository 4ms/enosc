#include "dynamic_data.hh"


Buffer<std::pair<s1_15, s1_15>, sine_size> DynamicData::sine;
Buffer<Buffer<f, cheby_size>, cheby_tables> DynamicData::cheby;

DynamicData::DynamicData() {

  // sine + difference
  MagicSine magic(1_f / f(sine_size-1));
  s1_15 previous = s1_15::inclusive(magic.Process());
  for (auto& [v, d] : sine) {
    v = previous;
    previous = s1_15::inclusive(magic.Process());
    d = previous - v;
  }

  // cheby[1] = [-1..1]
  for (int i=0; i<cheby_size; i++)
    cheby[0][i] = f(i * 2)/f(cheby_size-1) - 1_f;

  // cheby[2] = 2 * cheby[1] * cheby[1] - 1
  for (int i=0; i<cheby_size; i++)
    cheby[1][i] = 2_f * cheby[0][i] * cheby[0][i] - 1_f;

  // cheby[n] = 2 * cheby[1] * cheby[n-1] - cheby[n-2]
  for (int n=2; n<cheby_tables; n++)
    for (int i=0; i<cheby_size; i++)
      cheby[n][i] = 2_f * cheby[0][i] * cheby[n-1][i] - cheby[n-2][i];
}
