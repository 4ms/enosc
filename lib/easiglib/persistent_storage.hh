#pragma once

template <class T>
class Persistent : public T {
  static constexpr int size = sizeof(T);
public:
  Persistent(T const &default_data) : T(default_data) {
    // load into data_, failing back to default_data if not found
  }

  void Save() {
    // save
  }

  void Save(T const &data) {
    *this = data;
    Save();
  }
};
