#include <initializer_list>

#include "util.hh"
#include "signal.hh"
#include "numtypes.hh"

#pragma once

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


template<class T>
int linear_search(T const x, T const array[], int size) {
  while(size--) {
    if (x >= array[size]) return size;
  }
  return -1;
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
  T const& operator [] (unsigned int i) const {
    return data_[i];
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


template<class T, int SIZE>
struct Buffer : std::array<T, SIZE> {

  constexpr int size() const {return SIZE;}

  constexpr T& interpolate(f phase) const {
    phase *= f(size()-1);
    int integral = phase.floor();
    f fractional = phase.fractional();
    T a = (*this)[integral];
    T b = (*this)[integral+1];
    return Signal::crossfade(a, b, fractional);
  }

  constexpr T interpolate(u0_32 const phase) const {
    static_assert(is_power_of_2(SIZE-1), "only power-of-two-sized buffers");
    constexpr int BITS = Log2<SIZE>::val;
    Fixed<UNSIGNED, BITS, 32-BITS> p = phase.movr<BITS>();
    int integral = p.integral();
    auto fractional = p.fractional();
    T a = (*this)[integral];
    T b = (*this)[integral+1];
    return Signal::crossfade(a, b, fractional);
  }

  constexpr T interpolate(u0_32 const phase, Buffer<T, SIZE>& diff) const {
    static_assert(is_power_of_2(SIZE-1), "only power-of-two-sized buffers");
    constexpr int BITS = Log2<SIZE>::val;
    Fixed<UNSIGNED, BITS, 32-BITS> p = phase.movr<BITS>();
    int integral = p.integral();
    u0_32 fractional = p.fractional();
    T a = (*this)[integral];
    T d = diff[integral];
    return Signal::crossfade_with_diff(a, d, fractional);
  }

  constexpr T interpolate(u0_16 const phase) const {
    static_assert(is_power_of_2(SIZE-1), "only power-of-two-sized buffers");
    constexpr int BITS = Log2<SIZE>::val;
    Fixed<UNSIGNED, BITS, 16-BITS> p = phase.movr<BITS>();
    int integral = p.integral();
    auto fractional = p.fractional();
    T a = (*this)[integral];
    T b = (*this)[integral+1];
    return Signal::crossfade(a, b, fractional);
  }
};

template<typename T, int SIZE>
class RingBuffer {
  T buffer_[SIZE];
  const int S = SIZE;
  int cursor_ = S;
public:
  int size() { return S; }
  void Write(T& x) {
    ++cursor_;
    buffer_[cursor_ % S] = x;
  }
  T& Read(int n) {
    // TODO specialized version when S is 2^n
    return buffer_[(cursor_ - n) % S];
  }
  T& ReadLast() {
    return buffer_[(cursor_+1) % S];
  }
};

template<typename T>
class RingBuffer<T, 1> {
  T buffer_;
public:
  void Write(T x) { buffer_ = x; }
  void Read(int n) { return buffer_; }
  void ReadLast() { return buffer_; }
};

template<typename T, int SIZE>
class BufferReader {
  int delay_;
  RingBuffer<T, SIZE>& ring_buffer_;
public:
  BufferReader(RingBuffer<T, SIZE>& ring_buffer, int delay) :
    ring_buffer_(ring_buffer), delay_(delay) {}
  T& get(int n) {
    return ring_buffer_.Read(n+delay_);
  }
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
