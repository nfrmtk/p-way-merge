//
// Created by nfrmtk on 6/14/25.
//
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <pmerge/ydb/merge_reference.hpp>
#include <pmerge/ydb/merge_spilling_blocks.hpp>
#include <ranges>
#include <vector>

#include "gtest/gtest.h"
#include "pmerge/common/assert.hpp"
#include "pmerge/common/print.hpp"
#include "pmerge/ydb/spilling_mem.h"
#include "utils.hpp"

auto MakeRandomSpillBlocksDeque(TSpilling& stats) {
  std::deque<TSpillingBlock> external_memory_chunks;
  auto rnd = MakeRandomGenerator(0, (1 << 20) - 1);
  static constexpr int KeySize = 1;
  for (int _ = 0; _ < 16; ++_) {
    external_memory_chunks.emplace_back(pmerge::ydb::MakeSlotsBlock<KeySize>(
        stats, [&rnd]() { return rnd(); }, []() { return 1; }, 8));
  }

  return external_memory_chunks;
}
template <size_t KeySize, size_t TreeDepth>
auto MakeSpillBlocksDeque(TSpilling& stats, auto& keys_gen, auto& counts_gen,
                          auto& sizes_gen) {
  std::deque<TSpillingBlock> external_memory_chunks;

  for (int _ = 0; _ < (1 << TreeDepth); ++_) {
    external_memory_chunks.emplace_back(pmerge::ydb::MakeSlotsBlock<KeySize>(
        stats, [&]() { return keys_gen(); }, [&]() { return counts_gen(); },
        sizes_gen()));
  }
  return external_memory_chunks;
}

auto AsCharPointer(TSpillingBlock block) {
  return static_cast<const char*>(block.ExternalMemory);
}
size_t TotalSize(std::ranges::range auto&& blocks) {
  return std::accumulate(blocks.begin(), blocks.end(), 0,
                         [](int size_total, TSpillingBlock block) {
                           return size_total + block.BlockSize;
                         });
}
template <size_t KeySize, size_t TreeDepth>
void YDBInterfaceTest(auto keys_gen, auto counts_gen, auto sizes_gen) {
  TSpilling stats{128 * 1024 * 1024};
  std::deque<TSpillingBlock> external_memory_chunks =
      MakeSpillBlocksDeque<KeySize, TreeDepth>(stats, keys_gen, counts_gen,
                                               sizes_gen);
  auto chunks_copy =
      external_memory_chunks | std::views::transform([&](TSpillingBlock block) {
        return stats.Save(block.ExternalMemory, block.BlockSize, 0);
      }) |
      std::ranges::to<std::deque<TSpillingBlock>>();

  ASSERT_EQ(TotalSize(chunks_copy), TotalSize(external_memory_chunks));
  std::vector<uint64_t> buff;
  static const int kBufferSize = 3 * 1 << 13;
  static const int kBufferSizeSlots = kBufferSize / 8;
  buff.resize(kBufferSize, 0);
  size_t reference_writes =
      ydb_reference::merge2pway<false, KeySize, TreeDepth>(
          buff.data(), kBufferSizeSlots, stats, external_memory_chunks);
  buff.assign(kBufferSize, 0);
  size_t my_writes = pmerge::ydb::merge2pway<false, KeySize, TreeDepth>(
      buff.data(), kBufferSizeSlots, stats, chunks_copy);
  TSpillingBlock reference_block = external_memory_chunks.back();
  external_memory_chunks.pop_back();
  TSpillingBlock my_block = chunks_copy.back();
  chunks_copy.pop_back();

  Defer delete_reference = [&]() noexcept { stats.Delete(reference_block); };
  Defer delete_my = [&]() noexcept { stats.Delete(my_block); };

  ASSERT_EQ(my_writes, reference_writes);
  ASSERT_EQ(my_block.BlockSize, reference_block.BlockSize);
  if (my_block.BlockSize != 0) {
    ASSERT_STREQ(AsCharPointer(my_block), AsCharPointer(reference_block));
  }
}

using KeySize = size_t;
using Depth = size_t;

using ParamPair = std::pair<KeySize, Depth>;

using KeySizesAndDepths =
    ::testing::Types<std::integral_constant<ParamPair, {1, 1}>,
                     std::integral_constant<ParamPair, {2, 1}>,
                     std::integral_constant<ParamPair, {3, 1}>,
                     std::integral_constant<ParamPair, {4, 1}>,
                     std::integral_constant<ParamPair, {1, 2}>,
                     std::integral_constant<ParamPair, {2, 2}>,
                     std::integral_constant<ParamPair, {3, 2}>,
                     std::integral_constant<ParamPair, {4, 2}>,
                     std::integral_constant<ParamPair, {1, 3}>,
                     std::integral_constant<ParamPair, {2, 3}>,
                     std::integral_constant<ParamPair, {3, 3}>,
                     std::integral_constant<ParamPair, {4, 3}>,
                     std::integral_constant<ParamPair, {1, 4}>,
                     std::integral_constant<ParamPair, {2, 4}>,
                     std::integral_constant<ParamPair, {3, 4}>,
                     std::integral_constant<ParamPair, {4, 4}>>;
template <typename Pair>
class YDBInterfaceMerge : public ::UnmuteOnExitSuite {};

TYPED_TEST_SUITE(YDBInterfaceMerge, KeySizesAndDepths);

TYPED_TEST(YDBInterfaceMerge, Small) {
  constexpr size_t kKeySize = TypeParam::value.first;
  constexpr size_t kDepth = TypeParam::value.second;
  constexpr size_t kSize = 1;
  constexpr int kTotalKeyNumsNeeded = kKeySize * (1 << kDepth) * kSize;
  std::vector<int> nums{std::views::iota(0, kTotalKeyNumsNeeded) |
                        std::ranges::to<std::vector<int>>()};
  auto idx = 0;

  YDBInterfaceTest<kKeySize, kDepth>(
      [&]() {
        PMERGE_ASSERT(idx < nums.size());
        return nums[idx++];
      },
      []() { return 1; }, [&kSize] { return kSize; });
}

TYPED_TEST(YDBInterfaceMerge, Random) {
  constexpr size_t kKeySize = TypeParam::value.first;
  constexpr size_t kDepth = TypeParam::value.second;
  constexpr size_t kSize = 1;
  auto rnd_keys = MakeRandomGenerator(0, 1 << 20);
  auto rnd_counts = MakeRandomGenerator(0, 15);
  auto rnd_sizes = MakeRandomGenerator(50, 100);
  YDBInterfaceTest<kKeySize, kDepth>([&]() { return rnd_keys(); },
                                     [&]() { return rnd_counts(); },
                                     [&] { return rnd_sizes(); });
}
