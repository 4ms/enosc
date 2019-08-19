#include "dynamic_data.hh"


Buffer<std::pair<s1_15, s1_15>, sine_size> DynamicData::sine;
Buffer<Buffer<f, cheby_size>, cheby_tables> DynamicData::cheby;
Buffer<std::pair<f, f>, fold_size> DynamicData::fold;
Buffer<f, fold_size> DynamicData::fold_max;

DynamicData::DynamicData() {

  // sine + difference
  { MagicSine magic(1_f / f(sine_size-1));
    s1_15 previous = s1_15::inclusive(magic.Process());
    for (auto& [v, d] : sine) {
      v = previous;
      previous = s1_15::inclusive(magic.Process());
      d = previous - v;
    }
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

  // fold
  { f folds = 6_f;
    f previous = 0_f;
    for (int i=0; i<fold_size; ++i) {
      f x = folds * f(i+1) / f(fold_size);
      f g = 1_f / (1_f + x.abs());
      f p = 16_f / (2_f * Math::pi) * x * g;
      while(p > 1_f) p--;
      x = - g * (x + Math::fast_sine(p));
      fold[i] = std::pair(previous, x - previous);
      previous = x;
    }
  }

  // fold_max
  { f max = 0.001_f;
    for (int i=0; i<fold_size; ++i) {
      max = fold[i].first.abs().max(max);
      f val = 0.95_f / max;
      fold_max[i] = val;
    }
  }

  // // fold_max
  // { f max = fold[0].first;
  //   f previous = max;
  //   for (int i=0; i<fold_size; ++i) {
  //     max = max.max(fold[i+1].first);
  //     f val = 1_f / max;
  //     fold_max[i] = std::pair(previous, val - previous);
  //     previous = val;
  //   }
  // }
}
