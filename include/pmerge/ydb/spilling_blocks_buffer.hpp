//
// Created by nfrmtk on 6/14/25.
//

#ifndef SPILLING_BLOCKS_BUFFER_HPP
#define SPILLING_BLOCKS_BUFFER_HPP
#include <pmerge/ydb/spilling_mem.h>

#include <pmerge/common/assert.hpp>
#include <pmerge/ydb/types.hpp>
#include <span>
namespace pmerge::ydb {

class SpillingBlocksBuffer {
 public:
  SpillingBlocksBuffer(TSpilling& spilling, TSpillingBlock external_memory,
                       std::span<uint64_t> buffer)
      : spilling_(spilling),
        external_memory_(external_memory),
        buffer_(buffer) {
    PMERGE_ASSERT_M(external_memory_.BlockSize % buffer_.size_bytes() == 0,
                  "external memory should be spilttable by buffers");
    AssertSplittableBySlots(buffer);
  }
  std::span<uint64_t> ResetBuffer() {
    if (external_memory_.BlockSize == buffer_.size_bytes() * buffers_read_) {
      return {};
    }
    spilling_.Load(external_memory_, buffer_.size_bytes() * buffers_read_,
                   buffer_.data(), buffer_.size_bytes());
    ++buffers_read_;
    return buffer_;
  }

 private:

  TSpilling& spilling_;
  TSpillingBlock external_memory_;
  int64_t buffers_read_ = 0;
  std::span<uint64_t> buffer_;
};

}  // namespace pmerge::ydb

#endif  // SPILLING_BLOCKS_BUFFER_HPP
