#pragma once

template <class Storage>
class Persistent {
  using data_t = typename Storage::data_t;
  data_t* data_;
public:
  Persistent(data_t *data, data_t const &default_data) : data_(data) {
    // load to data_, falling back to default_data if not found
    bool success = Storage::Read(data_);
    success &= data->validate();
    if (!success) {
      *data = default_data;
    }
  }

  void Save() {
    Storage::Write(data_);
  }
};
