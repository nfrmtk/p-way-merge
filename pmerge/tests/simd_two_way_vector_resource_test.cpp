//
// Created by nfrmtk on 4/11/25.
//
#include <gtest/gtest.h>

#include <algorithm>
#include <bitset>
#include <pmerge/common/assert.hpp>
#include <pmerge/common/resource.hpp>
#include <pmerge/common/vector_resource.hpp>
#include <pmerge/simd/utils.hpp>
#include <pmerge/two_way/simd_two_way.hpp>
#include <random>

#include "utils.hpp"

static constexpr auto kFirstIdentifier = std::bitset<1>(0);

static constexpr auto kSecondIdentifier = std::bitset<1>(1);

using pmerge::common::FromDataAndIndex;

void TestMerge(const std::vector<int64_t>& first_vec,
               const std::vector<int64_t>& second_vec, bool print_registers) {
  std::vector<int64_t> merged;
  std::ranges::merge(first_vec, second_vec, std::back_inserter(merged));
  auto compare_and_push = [&, first_idx = 0,
                           second_idx = 0](std::bitset<1> index) mutable {
    if (index.to_ullong() == 0) {
      EXPECT_EQ(merged[first_idx + second_idx], first_vec[first_idx])
          << std::format("total_idx: {}, first_idx: {}", first_idx + second_idx,
                         first_idx);
      ++first_idx;
    } else {
      EXPECT_EQ(merged[first_idx + second_idx], second_vec[second_idx])
          << std::format("total_idx: {}, second_idx: {}",
                         first_idx + second_idx, second_idx);
      ++second_idx;
    }
  };
  pmerge::common::VectorResource res1{first_vec};
  pmerge::common::VectorResource res2{second_vec};
  pmerge::two_way::SimdTwoWayMerger merger{std::move(res1), std::move(res2),
                                           print_registers};
  if (merged.size() < 100) {
    for (auto num : merged) {
      pmerge::output << pmerge::MakeReadableString(num) << ' ';
    }
    pmerge::output << std::endl;
  }
  static_assert(pmerge::Resource<decltype(merger)>, "asd");
  for (int idx = 0; idx + 4 <= std::ssize(merged); idx += 4) {
    auto reg = merger.GetOne();
    auto arr = pmerge::simd::AsArray(reg);
    compare_and_push(*pmerge::ExtractIdentifer<1>(arr[0]));
    compare_and_push(*pmerge::ExtractIdentifer<1>(arr[1]));
    compare_and_push(*pmerge::ExtractIdentifer<1>(arr[2]));
    compare_and_push(*pmerge::ExtractIdentifer<1>(arr[3]));
  }
  auto last_arr = pmerge::simd::AsArray(merger.GetOne());
  int last_arr_idx = 0;
  while (last_arr_idx != 4 && last_arr[last_arr_idx] != pmerge::kInf) {
    compare_and_push(*pmerge::ExtractIdentifer<1>(last_arr[last_arr_idx]));
    last_arr_idx++;
  }
  ASSERT_EQ(merger.Peek(), pmerge::kInf)
      << "not all values from merger checked";
}

TEST(TwoWayMergeVectorResource, Simple) {
  TestMerge(FromDataAndIndex(std::vector<uint64_t>({1, 7, 9, 11, 32}),
                             kFirstIdentifier),
            FromDataAndIndex(std::vector<uint64_t>({2, 3, 4, 5, 32}),
                             kSecondIdentifier),
            true);
}

TEST(TwoWayMergeVectorResource, Random) {
  std::mt19937 random(123);
  std::vector<uint64_t> first, second;
  first.resize(1000);
  second.resize(1000);
  for (int _ = 0; _ < 128; ++_) {
    FillVector(first, random);
    FillVector(second, random);
    TestMerge(FromDataAndIndex(first, kFirstIdentifier),
              FromDataAndIndex(second, kSecondIdentifier), false);
  }
}
