#pragma once

template <class T>
class PersistentStorage : public T {
  static constexpr int size = sizeof(T);
public:
  PersistentStorage(T default_data) {
    *this = default_data;
    // load into data_, failing back to default_data if not found
  }

  void Save() {
    // save data_
  }
};
