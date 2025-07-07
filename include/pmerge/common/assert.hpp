//
// Created by nfrmtk on 6/14/25.
//
#ifndef ASSERT_HPP
#define ASSERT_HPP
#include <format>
#include <iostream>
#ifndef NDEBUG
#define PMERGE_ASSERT_M(condition, message)                              \
  do {                                                                   \
    if (!(condition)) {                                                  \
      std::cerr << std::format(                                          \
          "Assertion failed with message: '{}' at line: {}, file: {}\n", \
          message, __LINE__, __FILE__);                                  \
      abort();                                                           \
    }                                                                    \
  } while (0)
#define PMERGE_ASSERT(condition)                                           \
  do {                                                                     \
    if (!(condition)) {                                                    \
      std::cerr << std::format("Assertion failed at line: {}, file: {}\n", \
                               __LINE__, __FILE__);                        \
      abort();                                                             \
    }                                                                      \
  } while (0)

#else
#define PMERGE_ASSERT_M(condition, message) \
  {}

#define PMERGE_ASSERT(condition) \
  {}

#endif

#endif  // ASSERT_HPP
