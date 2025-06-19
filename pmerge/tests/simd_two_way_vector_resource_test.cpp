//
// Created by nfrmtk on 4/11/25.
//
#include <gtest/gtest.h>

#include <algorithm>
#include <pmerge/common/vector_resource.hpp>
#include <pmerge/simd/utils.hpp>
#include <pmerge/two_way/simd_two_way.hpp>
#include <random>

#include "utils.hpp"

void TestMerge(const std::vector<int64_t>& first_vec,
               const std::vector<int64_t>& second_vec) {
  pmerge::common::VectorResource res1(first_vec);
  std::vector<uint64_t> merged;
  std::ranges::merge(first_vec, second_vec, std::back_inserter(merged));
  pmerge::common::VectorResource res2(second_vec);
  pmerge::two_way::SimdTwoWayMerger merger{std::move(res1), std::move(res2)};
  static_assert(pmerge::Resource<decltype(merger)>, "asd");

  ASSERT_EQ(merger.Peek(), merged.front());

  for (int idx = 0; idx + 4 <= std::ssize(merged); idx += 4) {
    auto reg = merger.GetOne();
    auto arr = pmerge::simd::AsArray(reg);
    ASSERT_EQ(arr[0], merged[idx + 0]) << idx + 0;
    ASSERT_EQ(arr[1], merged[idx + 1]) << idx + 1;
    ASSERT_EQ(arr[2], merged[idx + 2]) << idx + 2;
    ASSERT_EQ(arr[3], merged[idx + 3]) << idx + 3;
  }
}

TEST(Merge, Simple) {
  TestMerge(std::vector<int64_t>({1, 7, 9, 11, 32}),
            std::vector<int64_t>({2, 3, 4, 5, 32}));
}

TEST(Merge, Random) {
  std::mt19937 random(123);
  std::vector<int64_t> first, second;
  first.resize(1000);
  second.resize(1000);
  for (int _ = 0; _ < 1000; ++_) {
    FillVector(first, random);
    FillVector(second, random);
    TestMerge(first, second);
  }
}
