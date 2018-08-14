#include <initializer_list>

#include "numtypes.hh"

using index = u32;

template<class T>
class Buffer {
  const T *data_;
  index size_;
public:
  constexpr Buffer(std::initializer_list<T> data) :
    data_(data.begin()), size_(data.size()) {};

  constexpr index size() const {return size_;}
  constexpr T operator[](index idx) const { return data_[idx.repr()]; }
};
