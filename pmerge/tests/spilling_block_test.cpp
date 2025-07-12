//
// Created by nfrmtk on 4/11/25.
//
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <pmerge/simd/utils.hpp>
#include <pmerge/two_way/simd_two_way.hpp>
#include <pmerge/ydb/spilling_block_resource.hpp>
#include <random>
#include <ranges>
#include <vector>

#include "gtest/gtest.h"
#include "pmerge/common/print.hpp"
#include "pmerge/common/resource.hpp"
#include "pmerge/utils/count.hpp"
#include "pmerge/ydb/spilling_mem.h"
#include "pmerge/ydb/types.hpp"
#include "utils.hpp"
static constexpr int kSpillBlockSize = 1 << 10;

constexpr int kExternalMemorySize = 1 << 15;

template <typename Number>
class SpillingResourceSuite : public ::testing::Test {};

using test_key_sizes = ::testing::Types<std::integral_constant<std::size_t, 1>,
                                        std::integral_constant<std::size_t, 2>,
                                        std::integral_constant<std::size_t, 3>,
                                        std::integral_constant<std::size_t, 4>>;

static constexpr int kRandomSeed = 123;
static constexpr int kBlockSize = 8 * 1024 * 1024;

template <int KeySize>
void TestSpillingBlockResource(auto keys, auto counts_gen, int64_t size_keys) {
  auto resource_index{std::bitset<1>{0}};
  int64_t current_debug_counter = debug_counter.Read();
  PMERGE_ASSERT_M(
      size_keys % (4 * KeySize) == 0,
      std::format("nums(size={}) must by splittable in groups of ({})",
                  size_keys, 4 * KeySize));
  TSpilling stats{kBlockSize};
  int cur = 0;
  int64_t size_slots = size_keys / KeySize;
  auto external_memory = pmerge::ydb::MakeSlotsBlock<KeySize>(
      stats, keys,
      counts_gen, size_slots);
  Defer delete_external_mem = [&]() noexcept { stats.Delete(external_memory); };
  auto buffer = MakeBuffer(4);
  std::vector<pmerge::IntermediateInteger> packed{
      SpillBlockToInts<KeySize>(stats, external_memory, resource_index)};
  std::ranges::sort(packed);
  pmerge::output << "packed: ";
  PrintIntermediateIntegersRange(packed);
  pmerge::ydb::SpillingBlockBufferedResource<1> resource{
      stats, external_memory, std::span{buffer}, resource_index};
  TestResource(resource, packed);
  ASSERT_EQ(debug_counter.Read(), current_debug_counter + std::ssize(packed));
}
template <int KeySize>
void TestSpillingBlockResourceWithNonZeroCounts(std::ranges::sized_range auto&& key_data) {
  int idx = 0;
  TestSpillingBlockResource<KeySize>([&]{return key_data[idx++];}, []{return 1;}, std::ssize(key_data));
}

TEST(SpillingBlockResource, simple) {
  TestSpillingBlockResourceWithNonZeroCounts<1>(std::vector{1, 3, 4, 8});
}

TYPED_TEST_SUITE(SpillingResourceSuite, test_key_sizes);

TYPED_TEST(SpillingResourceSuite, same_numbers) {
  static constexpr size_t kKeySize = TypeParam::value;
  TestSpillingBlockResourceWithNonZeroCounts<kKeySize>(std::views::repeat(1, kKeySize * 100));
}

TYPED_TEST(SpillingResourceSuite, increasing) {
  static constexpr size_t kKeySize = TypeParam::value;
  TestSpillingBlockResourceWithNonZeroCounts<kKeySize>(std::views::iota(0ull, 20 * kKeySize));
}

TYPED_TEST(SpillingResourceSuite, random) {
  static constexpr size_t kKeySize = TypeParam::value;
  std::mt19937_64 gen(123);
  std::uniform_int_distribution<int64_t> ints(0, 123);
  auto nums =
      std::views::repeat(0ull, 16 * kKeySize) |
      std::views::transform([&](size_t /*ignore*/) { return ints(gen); }) |
      std::ranges::to<std::vector<int64_t>>();
  TestSpillingBlockResourceWithNonZeroCounts<kKeySize>(nums);
}

TYPED_TEST(SpillingResourceSuite, ZeroeCounts){
  static constexpr size_t kKeySize = TypeParam::value;
  auto gen = MakeRandomGenerator(0, 1<<20);
  int64_t size = kKeySize * 100;
  auto ones = std::views::repeat(1, size);
  auto beg = ones.begin();
  TestSpillingBlockResource<kKeySize>([&beg]{return *beg++;}, []{return 0;}, size);

}

TYPED_TEST(SpillingResourceSuite, MixCounts){
  static constexpr size_t kKeySize = TypeParam::value;
  int64_t size = kKeySize*100;
  auto counts = MakeRandomGenerator(0,1);
  auto keys = std::views::repeat(1, size);
  int idx = 0;
  TestSpillingBlockResource<kKeySize>([&]{return keys[idx++];}, [&counts]{return static_cast<bool>(counts());}, size);
}

// TEST(Merge, Random) { TestMerge<uint32_t{2}>(); }
