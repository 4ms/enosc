#pragma once

template<class Storage>
class WearLevel {
  int cell_ = Storage::cell_nr_;
public:
  using data_t = typename Storage::data_t;
  static bool Read(data_t *data) {
    while(--cell_) {
      Storage::Read(data, cell);
      if (data.validate()) return true;
    }
    return false;
  }

  static bool Write(data_t *data) {
    return Storage::Write(data, cell++);
  }
};

template <class Storage>
class Persistent {
  using data_t = typename Storage::data_t;
  data_t* data_;
public:
  Persistent(data_t *data, data_t const &default_data) : data_(data) {
    // load to data_, falling back to default_data if not found
    if (!Storage::Read(data_) ||
        !data.validate()) {
      *data = default_data;
      Save();
    }
  }

  void Save() {
    Storage::Write(data_);
  }
};
