#pragma once
#include <format>
#include <iostream>
#include <ostream>
#include <string_view>
#include <utility>

namespace pmerge {
struct noop_ostr {
  void flush() {}
};
template <typename T>
noop_ostr& operator<<(noop_ostr& ostr, const T&) {
  return ostr;
}

inline noop_ostr& operator<<(noop_ostr& __os,
                             std::ostream& (*f)(std::ostream&)) {
  return __os;
}
namespace detail {
class Output {
 public:
  void Mute() { muted = true; }
  void Unmute() { muted = false; }

  friend Output& operator<<(Output& self, std::ostream& (*f)(std::ostream&));
  template <typename T>
  friend Output& operator<<(Output& self, const T& val);

 private:
  noop_ostr noop_;
  bool muted = false;
};
template <typename T>
Output& operator<<(Output& ostr, const T& val) {
#ifndef NDEBUG
  if (!ostr.muted) {
    std::cout << val;
  }
#endif
  return ostr;
}

inline Output& operator<<(Output& ostr, std::ostream& (*f)(std::ostream&)) {
#ifndef NDEBUG
  if (!ostr.muted) {
    std::cout << f;
  }
#endif
  return ostr;
}
}  // namespace detail
inline auto output = detail::Output{};

template <typename... Args>
void println(std::format_string<Args...> fmt, Args&&... args) {
#ifndef NDEBUG
  output << std::format(fmt, std::forward<Args>(args)...) << std::endl;
#endif
}

template <typename... Args>
void print(std::format_string<Args...> fmt, Args&&... args) {
#ifndef NDEBUG
  output << std::format(fmt, std::forward<Args>(args)...);
#endif
}

}  // namespace pmerge

namespace pmerge::utils {
inline void PrintIfDebug(std::string_view string) {
#ifndef NDEBUG
  pmerge::output << string;
  pmerge::output << std::endl;
#endif
}

void PrintIntermediateIntegersRange(std::ranges::range auto&& range) {
  println("[ ");
  for (const auto& num : range) {
    println("{}, ", num);
  }
  println("]");
}

}  // namespace pmerge::utils