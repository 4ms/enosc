#pragma once

template<class Storage>
class WearLevel : Storage {
  int cell_ = Storage::cell_nr_;
public:
  using data_t = typename Storage::data_t;

  bool Read(data_t *data) {
    while(--cell_) {
      Storage::Read(data, cell_);
      if (data->validate()) return true;
    }
    return false;
  }

  bool Write(data_t *data) {
    // TODO erase if cell_ = cell_nr_
    if (cell_ >= Storage::cell_nr_)
      Storage::Erase();
    return Storage::Write(data, cell_++);
  }
};

template <class Storage>
class Persistent : Storage {
  using data_t = typename Storage::data_t;
  data_t* data_;
public:
  Persistent(data_t *data, data_t const &default_data) : data_(data) {
    // load to data_, falling back to default_data if not found
    if (!Storage::Read(data_) ||
        !data_->validate()) {
      *data_ = default_data;
      Save();
    }
  }

  void Save() {
    Storage::Write(data_);
  }
};
