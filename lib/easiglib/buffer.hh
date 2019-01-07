#include <initializer_list>

#include "util.hh"
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
  constexpr Block(T* data) : data_(data) {}
  T& operator [] (unsigned int index) {
    return data_[index];
  }
  T const& operator [] (unsigned int index) const {
    return data_[index];
  }

  void fill(T x) { std::fill(data_, data_+SIZE, x); }
  T* const begin() const { return data_; }
  T* const end() const { return data_ + SIZE; }

  int size() {return SIZE; }
private:
  T *data_;
};

template<class T, class U, int SIZE>
struct DoubleBlock {
  struct iterator {
    iterator(T* x, U* y) : x_(x), y_(y) {}
    void operator++() { x_++; y_++; }
    bool operator!=(iterator &that) { return this->x_ != that.x_ || this->y_ != that.y_; }
    std::pair<T&, U&> operator*() { return std::pair<T&,U&>(*x_, *y_); }
  private:
    T *x_;
    U *y_;
  };

  iterator begin() {
    return iterator(x_,y_);
  }
  iterator end() {
    return iterator(x_+SIZE, y_+SIZE);
  }
public:
  DoubleBlock(T* x, U* y) : x_(x), y_(y) {}
  T *x_;
  U *y_;

  Block<T, SIZE> first() { return Block<T, SIZE>(x_); }
  Block<U, SIZE> second() { return Block<U, SIZE>(y_); }
};

template<class T, class U, class V, int SIZE>
struct TripleBlock {
  struct iterator {
    iterator(T* x, U* y, V* z) : x_(x), y_(y), z_(z) {}
    void operator++() { x_++; y_++; z_++; }
    bool operator!=(iterator &that) {
      return this->x_ != that.x_ || this->y_ != that.y_ || this->z_ != that.z_;
    }
    std::tuple<T&,U&,V&> operator*() { return std::tuple<T&,U&,V&>(*x_, *y_, *z_); }
  private:
    T *x_;
    U *y_;
    V *z_;
  };

  iterator begin() {
    return iterator(x_, y_, z_);
  }
  iterator end() {
    return iterator(x_+SIZE, y_+SIZE, z_+SIZE);
  }
public:
  TripleBlock(T* x, U* y, V* z) : x_(x), y_(y), z_(z) {}
  T *x_;
  U *y_;
  V *z_;

  Block<T, SIZE> first() { return Block<T, SIZE>(x_); }
  Block<U, SIZE> second() { return Block<U, SIZE>(y_); }
  Block<V, SIZE> third() { return Block<V, SIZE>(z_); }
};

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

  constexpr f interpolate(f phase) const {
    phase *= (size()-1_u32).to_float();
    index integral = index(phase);
    f fractional = phase - integral.to_float();
    f a = this->data_[integral];
    f b = this->data_[integral+1_u32];
    return a + (b - a) * fractional;
  }

  constexpr T interpolate(u0_32 const phase) const {
    static_assert(is_power_of_2(SIZE-1), "only power-of-two-sized buffers");
    constexpr int BITS = Log2<SIZE>::val;
    Fixed<UNSIGNED, BITS, 32-BITS> p = phase.movr<BITS>();
    u32 integral = p.integral();
    s1_15 frac = u0_16::narrow(p.fractional()).to_signed();
    T a = this->data_[integral];
    T b = this->data_[integral+1_u32];
    return a + s16::narrow((b-a) * frac);
  }

  constexpr f interpolate(u0_16 const phase) const {
    static_assert(is_power_of_2(SIZE-1), "only power-of-two-sized buffers");
    constexpr int BITS = Log2<SIZE>::val;
    Fixed<UNSIGNED, BITS, 16-BITS> p = phase.movr<BITS>();
    u32 integral = u32(p.integral());
    f frac = p.fractional().to_float();
    f a = this->data_[integral];
    f b = this->data_[integral+1_u32];
    return a + ((b-a) * frac);
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
