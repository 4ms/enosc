
#include <initializer_list>

// TODO -> u32
using index = unsigned int;

template<class T>
class Buffer {
  const T *data_;
  index size_;
public:
  constexpr Buffer(std::initializer_list<T> data) :
    data_(data.begin()), size_(data.size()) {};

  constexpr int size() const {return size_;}
  constexpr T operator[](index idx) const { return data_[idx]; }
};
