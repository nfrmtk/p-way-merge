//
// Created by nfrmtk on 6/13/25.
//
#include <gtest/gtest.h>

#include <cstdio>
#include <pmerge/common/resource.hpp>

TEST(BitPacking, Simple) {
  uint64_t num = 63;
  std::bitset<0> b = 0;
  ASSERT_EQ(pmerge::PackFrom(num, b), 31 + std::numeric_limits<int64_t>::min());
  ASSERT_EQ(pmerge::PackFrom(num, std::bitset<1>{0}) -
                std::numeric_limits<int64_t>::min(),
            30);
  ASSERT_EQ(pmerge::PackFrom(num, std::bitset<1>{1}) -
                std::numeric_limits<int64_t>::min(),
            31);
  ASSERT_EQ(pmerge::PackFrom(1ul << 3, std::bitset<2>(0)) -
                std::numeric_limits<int64_t>::min(),
            1ul << 2);
}

// 1000 vs 1