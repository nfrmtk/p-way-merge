//
// Created by nfrmtk on 6/15/25.
//

#ifndef SPILLING_BLOCK_READER_HPP
#define SPILLING_BLOCK_READER_HPP
#include <pmerge/ydb/spilling_blocks_buffer.hpp>

#include "pmerge/common/assert.hpp"
#include "pmerge/common/print.hpp"
#include "pmerge/common/resource.hpp"
#include "pmerge/simd/utils.hpp"
#include "pmerge/ydb/types.hpp"

namespace pmerge::ydb {

class SpillingBlockReader {
 public:
  explicit SpillingBlockReader(
      const SpillingBlocksBuffer &external_memory_cache, std::string name)
      : external_memory_cache_(external_memory_cache), name_(std::move(name)) {}
  std::optional<SlotView> GetNextValid() {
    if (!HasAnyData()) {
      return std::nullopt;
    }
    SlotView this_slot = AdvanceByOne();
    while (GetAggregateValue(this_slot) == 0 && HasAnyData()) {
      this_slot = AdvanceByOne();
    }
    if (GetAggregateValue(this_slot) == 0) {
      return std::nullopt;
    } else {
      return std::make_optional(this_slot);
    }
  }

 private:
  bool HasAnyData() const {
    pmerge::println("reader '{}' processed_bytes_: {}, total_: {}", name_,
                    processed_bytes_, external_memory_cache_.TotalBytes());
    return processed_bytes_ != external_memory_cache_.TotalBytes();
  }
  SlotView AdvanceByOne() {
    pmerge::println("reader '{}', slots left in storage: {}", name_,
                    slots_not_read_.size() >> 3);
    if (slots_not_read_.empty()) {
      slots_not_read_ = external_memory_cache_.ResetBuffer();
      pmerge::println("reader '{}' read from external memory {} bytes", name_,
                      slots_not_read_.size_bytes());
      PMERGE_ASSERT_M(!slots_not_read_.empty(), "reading past-the-end block");
    }
    auto next = GetSlot(slots_not_read_);
    processed_bytes_ += next.size_bytes();
    pmerge::println("'{}' get another slot with hash: {}\n", name_,
                    GetHash(next));
    PMERGE_ASSERT(GetAggregateValue(next) != 0);
    return next;
  }

  int64_t processed_bytes_ = 0;
  SpillingBlocksBuffer external_memory_cache_;
  std::span<uint64_t> slots_not_read_;
  std::string name_;
};

}  // namespace pmerge::ydb
#endif  // SPILLING_BLOCK_READER_HPP
