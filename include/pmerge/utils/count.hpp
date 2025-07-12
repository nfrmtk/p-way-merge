#pragma once

#include <cstdint>
class Count {
 public:
  Count() = default;
  void Inc() { value_++; }
  int64_t Read() { return value_; }

 private:
  int64_t value_;
};

inline static Count debug_counter;