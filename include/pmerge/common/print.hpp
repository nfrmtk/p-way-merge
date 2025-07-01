#include <iostream>
#include <string_view>
namespace pmerge::utils {
inline void PrintIfDebug(std::string_view string) {
#ifndef NDEBUG
  std::cout << string;
  std::cout.flush();
#endif
}
}  // namespace pmerge::utils