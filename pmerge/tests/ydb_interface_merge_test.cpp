//
// Created by nfrmtk on 6/14/25.
//
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <pmerge/ydb/merge_reference.hpp>
#include <pmerge/ydb/merge_spilling_blocks.hpp>
#include <ranges>
#include <vector>

#include "pmerge/ydb/spilling_mem.h"
#include "utils.hpp"

auto MakeSpillBlocksDeque(TSpilling& stats) {
  std::deque<TSpillingBlock> external_memory_chunks;
  auto rnd = MakeRandomGenerator(0, (1 << 20) - 1);
  static constexpr int KeySize = 1;
  for (int _ = 0; _ < 16; ++_) {
    external_memory_chunks.emplace_back(pmerge::ydb::MakeSlotsBlock<KeySize>(
        stats, [&rnd]() { return rnd(); }, []() { return 1; }, 4));
  }

  return external_memory_chunks;
}

auto AsCharPointer(TSpillingBlock block) {
  return static_cast<const char*>(block.ExternalMemory);
}

TEST(YDBMergeInterface, Random) {
  TSpilling stats{128 * 1024 * 1024};
  std::deque<TSpillingBlock> external_memory_chunks =
      MakeSpillBlocksDeque(stats);
  auto chunks_copy =
      external_memory_chunks | std::views::transform([&](TSpillingBlock block) {
        return stats.Save(block.ExternalMemory, block.BlockSize, 0);
      }) |
      std::ranges::to<std::deque<TSpillingBlock>>();
  std::vector<uint64_t> buff;
  static const int kBufferSize = 3 * 1 << 13;
  buff.resize(kBufferSize);
  ydb_reference::merge2pway<false, 1, 4>(buff.data(), buff.size(), stats,
                                         external_memory_chunks);
  buff.assign(kBufferSize, 0);
  pmerge::ydb::merge2pway<false, 1, 4>(buff.data(), buff.size(), stats,
                                       chunks_copy);
  TSpillingBlock reference_block = external_memory_chunks.back();
  external_memory_chunks.pop_back();
  TSpillingBlock my_block = chunks_copy.back();
  chunks_copy.pop_back();
  Defer delete_reference = [&]() noexcept { stats.Delete(reference_block); };
  Defer delete_my = [&]() noexcept { stats.Delete(my_block); };

  ASSERT_EQ(my_block.BlockSize, reference_block.BlockSize);

  ASSERT_STREQ(AsCharPointer(my_block), AsCharPointer(reference_block));
}