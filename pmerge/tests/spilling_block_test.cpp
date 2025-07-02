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
  ASSERT_TRUE(std::ranges::is_sorted(answer)) << RangeToString(answer);
  int answer_index = 0;
  const int answer_size = std::ranges::size(answer);

  while (tested_resouce.Peek() != pmerge::kInf) {
    pmerge::IntermediateInteger peeked = tested_resouce.Peek();
    int arr_index = 0;
    std::cout << "TestResource::GetOneCall" << std::endl;
    std::array<pmerge::IntermediateInteger, 4> arr =
        pmerge::simd::AsArray(tested_resouce.GetOne());

    auto debug_str = [&] {
      return std::format("got from resource: {}, vs expected: {}. ",
                         pmerge::MakeReadableString(arr[arr_index]),
                         pmerge::MakeReadableString(answer[answer_index]));
    };
    ASSERT_EQ(peeked, arr[0]);
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

void TestSpillingBlockWithKeySizeOne(std::ranges::sized_range auto&& nums) {
  constexpr int rem = 50 % 4;
  PMERGE_ASSERT_M(std::ssize(nums) % 4 == 0,
                  std::format("nums(size={}) must by splittable in groups of 4",
                              std::ssize(nums)));
  TSpilling stats{kBlockSize};
  auto external_memory = pmerge::ydb::MakeSlotsBlock<1>(
      stats, [cur = int{}, &nums]() mutable { return nums[cur++]; },
      [] { return 0; }, nums.size());
  Defer delete_external_mem = [&]() noexcept { stats.Delete(external_memory); };
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
template <int KeySize>
void TestSpillingBlockResource(std::ranges::sized_range auto&& key_data) {
  PMERGE_ASSERT_M(
      std::ssize(key_data) % (4 * KeySize) == 0,
      std::format("nums(size={}) must by splittable in groups of ({})",
                  std::ssize(key_data), 4 * KeySize));
  TSpilling stats{kBlockSize};
  auto external_memory = pmerge::ydb::MakeSlotsBlock<KeySize>(
      stats, [cur = int{}, &key_data]() mutable { return key_data[cur++]; },
      [] { return 0; }, key_data.size() / KeySize);
  Defer delete_external_mem = [&]() noexcept { stats.Delete(external_memory); };
  auto buffer = MakeBuffer(4);
  auto resource_index = std::bitset<1>{0};
  std::vector<pmerge::IntermediateInteger> packed{
      key_data | std::views::chunk(KeySize) |
      std::views::transform([&](auto nums) {
        std::array<uint64_t, KeySize> keys{{}};
        for (int idx = 0; idx < KeySize; ++idx) {
          keys[idx] = nums[idx];
        }
        return pmerge::PackFrom(
            pmerge::ydb::Hash(pmerge::ydb::Key<KeySize>{keys}), resource_index);
      }) |
      std::ranges::to<std::vector<pmerge::IntermediateInteger>>()};
  std::ranges::sort(packed);
  PrintIntermediateIntegersRange(packed);
  pmerge::ydb::SpillingBlockBufferedResource<1> resource{
      stats, external_memory, std::span{buffer}, resource_index};
  TestResource(resource, packed);
}

TEST(SpillingBlockResource, simple) {
  TestSpillingBlockWithKeySizeOne(std::vector{1, 3, 4, 8});
}

TEST(SpillingBlockResource, same_numbers) {
  TestSpillingBlockWithKeySizeOne(std::views::repeat(1, 100));
}

TEST(SpillingBlockResource, increasing) {
  TestSpillingBlockResource<2>(std::views::iota(0, 40));
  TestSpillingBlockWithKeySizeOne(std::views::iota(0, 40));
}

// TEST(Merge, Random) { TestMerge<uint32_t{2}>(); }
