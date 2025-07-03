#pragma once
#include <iostream>
#include <ostream>
#include <string_view>

#if 1
namespace pmerge{
inline auto& output =   std::cout;
}
#else
struct noop_ostr{
  void flush(){}
};
template<typename T>
noop_ostr& operator<<(noop_ostr& ostr, const T& ){return ostr;}

inline noop_ostr&
operator<<(noop_ostr& __os, std::ostream&(*f)(std::ostream&)){
 return __os; }
namespace pmerge{
inline auto output = noop_ostr{};
}
#endif


namespace pmerge::utils {
inline void PrintIfDebug(std::string_view string) {
  pmerge::output << string;
  pmerge::output.flush();
}
}  // namespace pmerge::utils