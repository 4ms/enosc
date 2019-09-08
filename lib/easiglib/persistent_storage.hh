#pragma once

template <class Data, class Storage>
class Persistent {
  Data* data_;
  static constexpr int size_ = sizeof(Data);
  static_assert(size_ < Storage::size_);
public:
  Persistent(Data *data, Data const &default_data) : data_(data) {
    // load to data_, falling back to default_data if not found
    bool success = Storage::Read(reinterpret_cast<uint8_t*>(data_), size_);
    success &= data->validate();
    if (!success) {
      *data = default_data;
    }
  }

  void Save() {
    Storage::Write(reinterpret_cast<uint8_t*>(data_), size_);
  }
};
