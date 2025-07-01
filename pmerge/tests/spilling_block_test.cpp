//
// Created by nfrmtk on 4/11/25.
//
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <memory>
#include <pmerge/simd/utils.hpp>
#include <pmerge/two_way/simd_two_way.hpp>
#include <pmerge/ydb/spilling_block_resource.hpp>
#include <random>
#include <ranges>
#include <vector>

#include "pmerge/common/resource.hpp"
#include "pmerge/ydb/spilling_mem.h"
#include "pmerge/ydb/types.hpp"
#include "utils.hpp"
static constexpr int kSpillBlockSize = 1 << 10;

constexpr int kExternalMemorySize = 1 << 15;

void TestResource(pmerge::Resource auto& tested_resouce,
                  std::ranges::random_access_range auto&& answer) {
  int answer_index = 0;
  const int answer_size = std::ranges::size(answer);

  while (tested_resouce.Peek() != pmerge::kInf) {
    pmerge::IntermediateInteger peeked = tested_resouce.Peek();
    int arr_index = 0;
    std::cout << "TestResource::GetOneCall" << std::endl;
    std::array<pmerge::IntermediateInteger, 4> arr =
        pmerge::simd::AsArray(tested_resouce.GetOne());
    
    auto debug_str = [&]{
      return std::format( "got from resource: {}, vs expected: {}. ", pmerge::MakeReadableString(arr[arr_index]), pmerge::MakeReadableString(answer[answer_index]));
    };
    ASSERT_EQ(peeked,arr[0]);
    while (arr_index != 4 && arr[arr_index] != pmerge::kInf) {
      ASSERT_LT(answer_index, answer_size);
      ASSERT_TRUE(pmerge::IsValid(arr[arr_index]));
      ASSERT_TRUE(pmerge::IsValid(answer[answer_index]));
      ASSERT_EQ(arr[arr_index], answer[answer_index]) << debug_str();
      ++answer_index;
      ++arr_index;
    }
  }
  ASSERT_EQ(answer_index, answer_size);
}

static constexpr int kRandomSeed = 123;
static constexpr int kBlockSize = 8 * 1024 * 1024;

void TestSpillingBlockWithKeySizeOne(std::ranges::sized_range auto&& nums){
  TSpilling stats{kBlockSize};
  auto external_memory = pmerge::ydb::MakeSlotsBlock<1>(
      stats, [cur = int{}, &nums] () mutable { return nums[cur++]; },
      [] { return 0; }, nums.size());
  Defer delete_external_mem = [&] () noexcept {
    stats.Delete(external_memory);
  };
  auto buffer = MakeBuffer(4);
  auto resource_index = std::bitset<1>{0};
  auto expected = nums | std::views::transform([&](int num) {
                           return pmerge::PackFrom(num, resource_index);
                         });
  PrintIntermediateIntegersRange(expected);
  pmerge::ydb::SpillingBlockBufferedResource<1> resource{
      stats, external_memory, std::span{buffer}, resource_index};
  TestResource(resource, expected);

}



TEST(SpillingBlockResource, simple) {
  TestSpillingBlockWithKeySizeOne(std::vector{1, 3, 4, 8, 13, 22, 23, 24});
}



// TEST(Merge, Random) { TestMerge<uint32_t{2}>(); }
