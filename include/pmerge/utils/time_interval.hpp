#include <chrono>
#include <iostream>
class TimeInterval {
 public:
  std::chrono::nanoseconds Reset() {
    auto now = std::chrono::steady_clock::now();
    auto diff = now - previous_;
    previous_ = now;
    return diff;
  }

 private:
  std::chrono::time_point<std::chrono::steady_clock> previous_{};
};

class WriteInterval {
 public:
  WriteInterval() { interval_.Reset(); }
  void Write8192Happend() {
    std::cout << std::format("8192 writes done in {}", interval_.Reset())
              << std::endl;
  }

 private:
  TimeInterval interval_{};
};