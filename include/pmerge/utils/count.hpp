#pragma once

#include <cstdint>
class DebugCount {
 public:
  DebugCount() = default;
  void Inc() { value_++; }
  int64_t Read() { return value_; }

 private:
  int64_t value_;
};

inline static DebugCount debug_counter;