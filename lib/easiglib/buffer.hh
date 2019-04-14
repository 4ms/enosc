#include <initializer_list>

#include "util.hh"
#include "signal.hh"
#include "numtypes.hh"

#pragma once

using index = u32;

template<class T>
int binary_search(T const x, T const array[], int const size) {
  int low = 0;
  int high = size-1;

  while (low+1 < high) {
    int mid = (low + high) / 2;
    if (x < array[mid]) {
      high = mid;
    } else {
      low = mid;
    }
  }
  return low;
}

// precondition: sorted [array] of [size]
// postcondition: deleted all elements closer than [threshold] to
// their predecessor
template<class T>
void uniquify(T array[], int &size, T threshold) {
  int j=1;
  for(int i=1; i<size; i++)
    if (array[i] - array[i-1] > threshold)
      array[j++] = array[i];
  size = j;
}

template<class T>
struct Table {
  constexpr Table(std::initializer_list<T> data) :
    data_(data.begin()) {};
  constexpr T operator[](index idx) const { return this->data_[idx.repr()]; }
  constexpr T operator[](size_t idx) const { return this->data_[idx]; }

protected:
  const T *data_;
};

template<class T, int SIZE>
struct Block {
  using value_type = T;
  struct iterator {
    iterator(T* x) : x_(x) {}
    void operator++() { x_++; }
    bool operator!=(iterator &that) { return this->x_ != that.x_; }
    value_type& operator*() { return *x_; }
  private:
    T *x_;
  };

  constexpr Block(T* data) : data_(data) {}
  T& operator [] (unsigned int index) {
    return data_[index];
  }
  T const& operator [] (unsigned int index) const {
    return data_[index];
  }

  void fill(T x) { std::fill(data_, data_+SIZE, x); }
  iterator const begin() const { return data_; }
  iterator const end() const { return data_ + SIZE; }

  T* data() { return data_; }
  int size() { return SIZE; }
private:
  T *data_;
};

// must have a tuple_size implementation
template<int SIZE, class T>
struct std::tuple_size<Block<T, SIZE>> { static constexpr int value = SIZE; };


// TODO: size not linked to data
template<class T, unsigned int SIZE>
struct Buffer {
private:
  Table<T> data_;
public:

  constexpr Buffer(std::initializer_list<T> data) : data_(data) {};

  constexpr index size() const {return index::of_long_long(SIZE);}
  constexpr T operator[](index idx) const { return this->data_[idx]; }
  constexpr T operator[](size_t idx) const { return this->data_[idx]; }

  // zero-order hold
  constexpr T operator[](f phase) const {
    phase *= (size()-1_u32).to_float();
    index integral = index(phase);
    return this->data_[integral];;
  }

  // zero-order hold
  constexpr T operator[](u0_32 const phase) const {
    static_assert(is_power_of_2(SIZE-1), "only power-of-two-sized buffers");
    constexpr int BITS = Log2<SIZE>::val;
    index i = phase.movr<BITS>().integral();
    return this->data_[i];
  }

  constexpr T interpolate(f phase) const {
    phase *= (size()-1_u32).to_float();
    index integral = index(phase);
    f fractional = phase.fractional();
    T a = this->data_[integral];
    T b = this->data_[integral+1_u32];
    return Signal::crossfade(a, b, fractional);
  }

  constexpr T interpolate(u0_32 const phase) const {
    static_assert(is_power_of_2(SIZE-1), "only power-of-two-sized buffers");
    constexpr int BITS = Log2<SIZE>::val;
    Fixed<UNSIGNED, BITS, 32-BITS> p = phase.movr<BITS>();
    u32 integral = p.integral();
    auto fractional = p.fractional();
    T a = this->data_[integral];
    T b = this->data_[integral+1_u32];
    return Signal::crossfade(a, b, fractional);
  }

  constexpr T interpolate(u0_16 const phase) const {
    static_assert(is_power_of_2(SIZE-1), "only power-of-two-sized buffers");
    constexpr int BITS = Log2<SIZE>::val;
    Fixed<UNSIGNED, BITS, 16-BITS> p = phase.movr<BITS>();
    u32 integral = u32(p.integral());
    auto fractional = p.fractional();
    T a = this->data_[integral];
    T b = this->data_[integral+1_u32];
    return Signal::crossfade(a, b, fractional);
  }
};

template<typename T, unsigned int SIZE>
class RingBuffer {
  T buffer_[SIZE];
  static constexpr index S = index::of_repr(SIZE);
  index cursor_ = S;
public:
  index size() { return S; }
  void Write(T x) {
    ++cursor_;
    index idx = cursor_ % S;
    buffer_[(idx).repr()] = x;
  }
  T Read(index n) {
    // TODO specialized version when S is 2^n
    return buffer_[((cursor_ - n) % S).repr()];
  }
  T ReadLast() {
    return buffer_[((cursor_+1_u32) % S).repr()];
  }
};

template<typename T>
class RingBuffer<T, 1> {
  T buffer_;
public:
  void Write(T x) { buffer_ = x; }
  void Read(index n) { return buffer_; }
  void ReadLast() { return buffer_; }
};

// WARNING: untested
template <class T, int SIZE>
class Queue {
  T buf_[SIZE];
  int head_ = 0;
  int tail_ = 0;
  bool full_ = false;
public:
  bool put(T item) {
    if(full_) return false;
    buf_[head_] = item;
    head_ = (head_+1) % SIZE;
    full_ = head_ == tail_;
    return true;
  }

  bool get(T& x) {
    if(empty()) return false;
    x = buf_[tail_];
    full_ = false;
    tail_ = (tail_+1) % SIZE;
    return true;
  }

  T get() { T x; get(x); return x; }
  void reset() { head_ = tail_; full_ = false; }
  bool empty() const { return (!full_ && (head_ == tail_)); };
  bool full() const { return full_; };
};
