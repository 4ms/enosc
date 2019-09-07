#pragma once

template <class T>
class Persistent {
  T& data_;
  static constexpr int size = sizeof(T);
public:
  Persistent(T &data, T const &default_data) : data_(data) {
    // load to data_, falling back to default_data if not found
    if (!data.validate()) {
      data = default_data;
    }
  }

  void Save() {
    // save
  }
};
