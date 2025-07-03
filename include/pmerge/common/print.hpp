#pragma once
#include <iostream>
#include <ostream>
#include <string_view>

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
class Output{

  public:
  void Mute(){
    muted = true;
  }
  void Unmute(){
    muted = false;
  }

  friend Output& operator<< (Output& self, std::ostream& (*f)(std::ostream&));
  template<typename T> 
  friend Output& operator<< (Output& self, const T& val);
private:
  noop_ostr noop_;
  bool muted = false;
};
template <typename T>
Output& operator<<(Output& ostr, const T& val) {
  if (!ostr.muted){
    std::cout << val;
  }
  return ostr;
}

inline Output& operator<<(Output& ostr,
                             std::ostream& (*f)(std::ostream&)) {
  if (!ostr.muted){
    std::cout << f;
  }
  return ostr;
}

inline auto output = Output{};
}

namespace pmerge::utils {
inline void PrintIfDebug(std::string_view string) {
#ifndef NDEBUG
  pmerge::output << string;
  pmerge::output << std::endl;
#endif
}
}  // namespace pmerge::utils