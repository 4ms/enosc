#pragma once

template <class T>
class Persistent {
  T& data_;
  static constexpr int size = sizeof(T);
public:
  Persistent(T &data, T const &default_data) : data_(data) {
    // load to data_, failing back to default_data if not found
    data = default_data;
  }

  void Save() {
    // save
  }
};
