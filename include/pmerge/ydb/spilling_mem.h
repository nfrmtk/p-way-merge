#pragma once

#include <pmerge/ydb/types.hpp>
#include <cassert>

class TSpillingBlock {
public:
  TSpillingBlock(void* external_memory, ui64 block_size, ui64 data):ExternalMemory(external_memory), BlockSize(block_size), Data(data) {
    PMERGE_ASSERT_M(BlockSize%64 == 0, std::format("BlockSize(={}) must be splittable in pmerge::ydb::Slot's (sizeof(Slot) == 64)", BlockSize));
  }

    void* ExternalMemory;
    ui64 BlockSize;
    ui64 Data; // data is unused, dont know what it is.
};



class TSpilling { // stats class

public:
    explicit TSpilling(ui64 chunkSize) : ChunkSize(chunkSize) {

    }

    TSpillingBlock Empty(ui64 data);
    TSpillingBlock Save(const void* source_buffer, ui64 size, ui64 data);
    TSpillingBlock Append(TSpillingBlock block, const void* source_buffer, ui64 size);
    void Load(TSpillingBlock source_block, ui64 source_offset, void* output_buffer, ui64 size);
    void Delete(TSpillingBlock block);

    ui64 ChunkSize;
    ui64 WriteChunkCount = 0;
    ui64 ReadChunkCount = 0;
    ui64 CurrentChunkCount = 0;
    ui64 MaxChunkCount = 0;
};
