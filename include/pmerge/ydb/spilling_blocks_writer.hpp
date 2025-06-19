//
// Created by nfrmtk on 6/18/25.
//

#ifndef SPILLING_BLOCKS_WRITER_HPP
#define SPILLING_BLOCKS_WRITER_HPP
#include <pmerge/ydb/spilling_mem.h>

#include <pmerge/ydb/types.hpp>
#include <span>

#include "pmerge/common/assert.hpp"
namespace pmerge::ydb {

class SpillingBlocksWriter {
public:
  SpillingBlocksWriter( TSpilling& stats ,TSpillingBlock external_memory, std::span<uint64_t> buffer):stats_(stats), external_memory_(external_memory), buffer_(buffer){}
  void Write(Slot slot) {
    total_writes_++;
    if (buffer_left_.empty()) {
      Flush();
    }

    buffer_left_ = slot;
    
    buffer_left_ = buffer_left_.subspan(8);
  }
  void Flush() {
    external_memory_ = stats_.Append(external_memory_, buffer_left_.data(), buffer_left_.size_bytes());
    buffer_left_ = buffer_;
  }
  ~SpillingBlocksWriter() {
    PMERGE_ASSERT_M(buffer_left_.size() == 0, "writer buffer dropping some slots not written to memory");
  }
  int64_t TotalWrites() const { return total_writes_; }
private:
  TSpilling& stats_;
  int64_t total_writes_;
  TSpillingBlock external_memory_;
  std::span<uint64_t> buffer_left_;
  const std::span<uint64_t> buffer_;
};

}
#endif //SPILLING_BLOCKS_WRITER_HPP
