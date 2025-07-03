//
// Created by nfrmtk on 6/15/25.
//

#ifndef SPILLING_BLOCK_READER_HPP
#define SPILLING_BLOCK_READER_HPP
#include <pmerge/ydb/spilling_blocks_buffer.hpp>

#include "pmerge/common/resource.hpp"
#include "pmerge/simd/utils.hpp"
#include "pmerge/ydb/types.hpp"

namespace pmerge::ydb {

class SpillingBlockReader {
 public:
  explicit SpillingBlockReader(
      const SpillingBlocksBuffer &external_memory_cache)
      : external_memory_cache_(external_memory_cache) {}
  SlotView AdvanceByOne() {
    pmerge::output << std::format("slots left in storage: {}",
                             slots_not_read_.size() >> 3)
              << std::endl;
    if (slots_not_read_.empty()) {
      slots_not_read_ = external_memory_cache_.ResetBuffer();
      PMERGE_ASSERT_M(!slots_not_read_.empty(), "reading past-the-end block");
    }
    auto next = GetSlot(slots_not_read_);
    pmerge::output << std::format("Get another slot with hash: {:x}\n",
                             GetHash(next));
    return next;
  }

 private:
  SpillingBlocksBuffer external_memory_cache_;
  std::span<uint64_t> slots_not_read_;
};

}  // namespace pmerge::ydb
#endif  // SPILLING_BLOCK_READER_HPP
