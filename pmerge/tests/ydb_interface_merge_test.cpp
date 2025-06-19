//
// Created by nfrmtk on 6/14/25.
//
#include <gtest/gtest.h>

#include <pmerge/ydb/merge_spilling_blocks.hpp>

TEST(YDBMergeInterface, Basic) {
  TSpilling stats{128 * 1024 * 1024};
  std::deque<TSpillingBlock> external_memory_chunks;
  pmerge::ydb::merge2pway<false, 1, 3>(nullptr, 123, std::cout, stats,
                                       external_memory_chunks);
}