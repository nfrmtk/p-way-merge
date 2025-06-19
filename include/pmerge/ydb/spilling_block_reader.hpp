//
// Created by nfrmtk on 6/15/25.
//

#ifndef SPILLING_BLOCK_READER_HPP
#define SPILLING_BLOCK_READER_HPP
#include <pmerge/ydb/spilling_blocks_buffer.hpp>

namespace pmerge::ydb {

class SpillingBlockReader {
 public:
  explicit SpillingBlockReader(const SpillingBlocksBuffer &external_memory_cache)
      : external_memory_cache_(external_memory_cache) {}
  Slot AdvanceByOne() {
      if (slots_not_read_.empty()) {
        slots_not_read_ = external_memory_cache_.ResetBuffer();
        PMERGE_ASSERT_M(!slots_not_read_.empty(), "reading past-the-end block");
      }
      return GetSlot(slots_not_read_);
  }

 private:
  SpillingBlocksBuffer external_memory_cache_;
  std::span<uint64_t> slots_not_read_;
};

}  // namespace pmerge::ydb
#endif  // SPILLING_BLOCK_READER_HPP
