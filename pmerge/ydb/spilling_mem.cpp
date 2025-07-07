#include <pmerge/ydb/spilling_mem.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <pmerge/common/assert.hpp>
#include <thread>
TSpillingBlock TSpilling::Empty(ui64 data) {
  TSpillingBlock block{nullptr, 0, data};
  return block;
}

TSpillingBlock TSpilling::Save(const void* source_buffer, ui64 size,
                               ui64 data) {
  TSpillingBlock block{malloc(size), size, data};
  std::memcpy(block.ExternalMemory, source_buffer, size);

  auto chunkCount = (size + (ChunkSize - 1)) / ChunkSize;
  WriteChunkCount += chunkCount;
  CurrentChunkCount += chunkCount;
  MaxChunkCount = std::max(MaxChunkCount, CurrentChunkCount);

  return block;
}

TSpillingBlock TSpilling::Append(TSpillingBlock currentBlock,
                                 const void* buffer, ui64 size) {
  auto newSize = currentBlock.BlockSize + size;
  TSpillingBlock block{realloc(currentBlock.ExternalMemory, newSize), newSize,
                       currentBlock.Data};
  std::memcpy(static_cast<char*>(block.ExternalMemory) + currentBlock.BlockSize,
              buffer, size);

  auto currentChunkCount =
      (currentBlock.BlockSize + (ChunkSize - 1)) / ChunkSize;
  auto newChunkCount = (newSize + (ChunkSize - 1)) / ChunkSize;
  WriteChunkCount += (size + (ChunkSize - 1)) / ChunkSize;
  CurrentChunkCount += newChunkCount - currentChunkCount;
  MaxChunkCount = std::max(MaxChunkCount, CurrentChunkCount);
  return block;
}

void TSpilling::Load(TSpillingBlock block, ui64 offset, void* buffer,
                     ui64 size) {
  PMERGE_ASSERT_M(
      offset % 64 == 0 && size % 64 == 0,
      std::format(
          "Load only full blocks of 64 bytes, (got [{}, {}) bytes instead)",
          offset, offset + size));
  PMERGE_ASSERT_M(
      offset + size <= block.BlockSize,
      std::format("reading past the last element of TSpillingBlock(reading "
                  "range is [{};{}), size is {} )",
                  offset, offset + size, block.BlockSize));
  std::memcpy(buffer, static_cast<char*>(block.ExternalMemory) + offset, size);
  ReadChunkCount += (size + (ChunkSize - 1)) / ChunkSize;
  std::this_thread::sleep_for(
      std::chrono::microseconds{20});  // dont read by 8 bytes
}

void TSpilling::Delete(TSpillingBlock block) {
  free(block.ExternalMemory);
  CurrentChunkCount -= (block.BlockSize + (ChunkSize - 1)) / ChunkSize;
}
