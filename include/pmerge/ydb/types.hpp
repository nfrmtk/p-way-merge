//
// Created by nfrmtk on 6/13/25.
//

#ifndef TYPES_HPP
#define TYPES_HPP
#include <cstdint>
#include <span>
#include <source_location>
#include <pmerge/common/assert.hpp>
typedef uint32_t ui32;
typedef int32_t i32;

typedef uint64_t ui64;
typedef int64_t i64;

namespace pmerge::ydb {
static constexpr int kSlotBytes = 64;
using Slot = std::span<uint64_t, 8>;
using ConstSlot = std::span<const uint64_t, 8>;

Slot GetSlot(std::span<uint64_t>& buffer) {
  Slot ret = buffer.subspan<0,8>();
  buffer = buffer.subspan<8>();
  return ret;
}

ConstSlot GetConstSlot(std::span<const uint64_t>& buffer) {
  ConstSlot ret = buffer.subspan<0,8>();
  buffer = buffer.subspan<8>();
  return ret;
}


inline void AssertSplittableBySlots(std::span<const uint64_t> buffer, std::source_location loc = std::source_location::current()) {
  PMERGE_ASSERT_M(buffer.size_bytes() % kSlotBytes == 0, loc.function_name());
}

inline uint64_t GetHash(Slot slot) { return slot[0]; }

inline uint64_t& GetAggregateValue(Slot slot) {
  return slot[7];
}
template<ui32 KeyCount>
std::span<const uint64_t, KeyCount> GetKey(Slot slot) {
  return {&slot[1], &slot[1+KeyCount]};
}




}  // namespace pmerge::ydb

#endif  // TYPES_HPP
