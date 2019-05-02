#pragma once

#include "util.hh"

struct Debug : Nocopy {
  Debug();
  void set(int pin, bool value);
};
