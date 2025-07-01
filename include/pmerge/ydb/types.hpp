//
// Created by nfrmtk on 6/13/25.
//

#ifndef TYPES_HPP
#define TYPES_HPP
#include <cstdint>
#include <pmerge/common/assert.hpp>
#include <source_location>
#include <span>
typedef uint32_t ui32;
typedef int32_t i32;

typedef uint64_t ui64;
typedef int64_t i64;

namespace pmerge::ydb {
struct Slot {
  uint64_t nums[8];
};

static constexpr int kSlotBytes = 64;
using SlotView = std::span<uint64_t, 8>;
using ConstSlotView = std::span<const uint64_t, 8>;

inline SlotView GetSlot(std::span<uint64_t>& buffer) {
  SlotView ret = buffer.subspan<0, 8>();
  buffer = buffer.subspan<8>();
  return ret;
}


inline ConstSlotView GetConstSlot(std::span<const uint64_t>& buffer) {
  ConstSlotView ret = buffer.subspan<0, 8>();
  buffer = buffer.subspan<8>();
  return ret;
}

inline void AssertSplittableBySlots(
    std::span<const uint64_t> buffer,
    std::source_location loc = std::source_location::current()) {
  PMERGE_ASSERT_M(buffer.size_bytes() % kSlotBytes == 0, loc.function_name());
}

inline uint64_t GetHash(ConstSlotView slot) { return slot[0]; }
inline uint64_t GetHash(const Slot& slot) { return slot.nums[0]; }
inline ConstSlotView AsView(const Slot& slot) { return slot.nums; }

inline uint64_t& GetAggregateValue(SlotView slot) { return slot[7]; }
template <ui64 KeyCount>
using Key = std::span<const uint64_t, KeyCount>;

template <ui64 KeyCount>
Key<KeyCount> GetKey(ConstSlotView slot) {
  return Key<KeyCount>{&slot[1], &slot[1 + KeyCount]};
}

}  // namespace pmerge::ydb

#endif  // TYPES_HPP
