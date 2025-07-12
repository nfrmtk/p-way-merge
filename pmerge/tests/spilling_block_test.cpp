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
void TestSpillingBlockResource(std::ranges::sized_range auto&& key_data) {
  auto resource_index{std::bitset<1>{0}};
  int64_t current_debug_counter = debug_counter.Read();
  PMERGE_ASSERT_M(
      std::ssize(key_data) % (4 * KeySize) == 0,
      std::format("nums(size={}) must by splittable in groups of ({})",
                  std::ssize(key_data), 4 * KeySize));
  TSpilling stats{kBlockSize};
  int cur = 0;
  int64_t size_slots = key_data.size() / KeySize;
  auto external_memory = pmerge::ydb::MakeSlotsBlock<KeySize>(
      stats, [&cur, &key_data]() mutable { return key_data[cur++]; },
      [] { return 1; }, size_slots);
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
  ASSERT_EQ(debug_counter.Read(), current_debug_counter + size_slots);
}

TEST(SpillingBlockResource, simple) {
  TestSpillingBlockResource<1>(std::vector{1, 3, 4, 8});
}

TYPED_TEST_SUITE(SpillingResourceSuite, test_key_sizes);

TYPED_TEST(SpillingResourceSuite, same_numbers) {
  static constexpr size_t kKeySize = TypeParam::value;
  TestSpillingBlockResource<kKeySize>(std::views::repeat(1, kKeySize * 100));
}

TYPED_TEST(SpillingResourceSuite, increasing) {
  static constexpr size_t kKeySize = TypeParam::value;
  TestSpillingBlockResource<kKeySize>(std::views::iota(0ull, 20 * kKeySize));
}

TYPED_TEST(SpillingResourceSuite, random) {
  static constexpr size_t kKeySize = TypeParam::value;
  std::mt19937_64 gen(123);
  std::uniform_int_distribution<int64_t> ints(0, 123);
  auto nums =
      std::views::repeat(0ull, 16 * kKeySize) |
      std::views::transform([&](size_t /*ignore*/) { return ints(gen); }) |
      std::ranges::to<std::vector<int64_t>>();
  TestSpillingBlockResource<kKeySize>(nums);
}

// TEST(Merge, Random) { TestMerge<uint32_t{2}>(); }
