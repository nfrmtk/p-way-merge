//
// Created by nfrmtk on 6/14/25.
//

#ifndef MERGE_SPILLING_BLOCKS_HPP
#define MERGE_SPILLING_BLOCKS_HPP
#include <algorithm>
#include <cstdint>
#include <deque>
#include <numeric>
#include <pmerge/multi_way/loser_tree.hpp>
#include <pmerge/ydb/spilling_block_reader.hpp>
#include <pmerge/ydb/spilling_block_resource.hpp>
#include <pmerge/ydb/spilling_blocks_writer.hpp>
#include <ranges>

#include "spilling_mem.h"

template <ui32 keyCount>
bool Equal(std::span<const pmerge::ydb::SlotView, keyCount> first,
           std::span<const pmerge::ydb::SlotView, keyCount> second) {
  for (int idx = 0; idx < keyCount; ++idx) {
    if (first[idx] != second[idx]) {
      return false;
    }
  }
  return true;
}

template <ui32 keyCount>
bool Less(std::span<const pmerge::ydb::SlotView, keyCount> first,
          std::span<const pmerge::ydb::SlotView, keyCount> second) {
  for (int idx = 0; idx < keyCount; ++idx) {
    if (first[idx] > second[idx]) {
      return false;
    }
    if (first[idx] < second[idx]) {
      return true;
    }
  }
  return false;
}

template <ui32 keyCount>
bool SlotLess(const pmerge::ydb::SlotView& first,
              const pmerge::ydb::SlotView& second) {
  if (pmerge::ydb::GetHash(first) < pmerge::ydb::GetHash(second)) {
    return true;
  }
  if (pmerge::ydb::GetHash(first) > pmerge::ydb::GetHash(second)) {
    return false;
  }
  auto first_span = pmerge::ydb::GetKey<keyCount>(first);
  auto second_span = pmerge::ydb::GetKey<keyCount>(second);
  if (Equal(first_span, second_span)) {
    return false;
  }
  if (Less(first_span, second_span)) {
    return true;
  } else {
    PMERGE_ASSERT(
        std::ranges::lexicographical_compare(second_span, first_span));
    return false;
  }
}
template <ui32 keyCount>
bool SlotEqual(const pmerge::ydb::SlotView& first,
               const pmerge::ydb::SlotView& second) {
  return pmerge::ydb::GetHash(first) == pmerge::ydb::GetHash(second) &&
         Equal(pmerge::ydb::GetKey<keyCount>(first),
               pmerge::ydb::GetKey<keyCount>(second));
}

namespace pmerge::ydb {
template <bool finalize /*use fo?*/, ui32 keyCount /*hashed string length*/,
          ui32 p /*loser tree depth*/>
ui32 merge2pway(ui64* wordsBuffer, ui32 wordsBufferSize, std::ostream& fo,
                TSpilling& sp, std::deque<TSpillingBlock>& spills) {
  int64_t slotBufferSize = wordsBufferSize / 8;
  int buffer_offset_slots = 0;
  auto make_buffer_with_size = [&](int size) -> std::span<ui64> {
    PMERGE_ASSERT(size > 0);
    PMERGE_ASSERT(buffer_offset_slots + size <= slotBufferSize);
    buffer_offset_slots += size;
    return {wordsBuffer + 8 * (buffer_offset_slots - size),
            wordsBuffer + 8 * buffer_offset_slots};
  };
  constexpr i32 n = 1ul << p;
  using Identifer = std::bitset<p>;
  uint64_t external_buffer_size = slotBufferSize / 3;
  auto input_buffer_size = external_buffer_size >> p;
  auto output_buffer_size = external_buffer_size >> p;
  SpillingBlocksWriter external_memory_buffer{
      sp, sp.Empty(0), make_buffer_with_size(external_buffer_size)};
  using TreeType =
      pmerge::multi_way::LoserTree<SpillingBlockBufferedResource<p>, 1 << p>;
  assert(spills.size() >= n);
  std::vector<pmerge::ydb::SpillingBlockReader> output_readers;
  for (int idx = 0; idx < n; ++idx) {
    output_readers.emplace_back(SpillingBlocksBuffer{
        sp, spills[idx], make_buffer_with_size(output_buffer_size)});
  }
  TreeType loser_tree = pmerge::multi_way::MakeLoserTree<1 << p>(
      std::views::iota(0, n) | std::views::transform([&](int block_index) {
        return SpillingBlockBufferedResource<p>(
            sp, spills[block_index], make_buffer_with_size(input_buffer_size),
            Identifer{block_index});
      }));
  std::deque<IntermediateInteger> nums_left;
  auto append_array = [&](const std::array<IntermediateInteger, 4>& arr) {
    nums_left.push_back(arr[0]);
    nums_left.push_back(arr[1]);
    nums_left.push_back(arr[2]);
    nums_left.push_back(arr[3]);
  };
  std::array<int64_t, 1 << p> amounts_of_slots_from{{}};
  std::vector<pmerge::ydb::SlotView> slots;
  auto reset_amounts_of_slots_from = [&] {
    amounts_of_slots_from.fill(0);
    int total_amount = 0;
    const uint64_t current_hash_bits =
        *pmerge::ExtractCutHash<p>(nums_left.front());
    size_t index = pmerge::ExtractIdentifer<p>(nums_left.front());
    slots.emplace_back(output_readers[index].AdvanceByOne());
    while (!nums_left.empty() &&
           *pmerge::ExtractCutHash<p>(nums_left.front()) == current_hash_bits) {
      index = pmerge::ExtractIdentifer<p>(nums_left.front())->to_ullong();
      ++amounts_of_slots_from[index];
      nums_left.pop_front();
      ++total_amount;
      slots.emplace_back(output_readers[index].AdvanceByOne());
    }
  };
  while (true) {
    if (loser_tree.template Peek() == kInf && nums_left.empty()) {
      break;
    }
    // if (!nums_left.empty()) {
    //   if (next == kInf) {
    //
    //   }
    //   if (next != kInf && ExtractCutHash<p>(nums_left.front()) ==
    //   ExtractCutHash<p>(next)) {
    //     auto arr = pmerge::simd::AsArray( loser_tree.template GetOne());
    //     append_array(arr);
    //   }
    // }

    while (nums_left.empty() ||
           ExtractCutHash<p>(nums_left.front()) ==
               ExtractCutHash<p>(loser_tree.template Peek())) {
      PMERGE_ASSERT_M(nums_left.front() != kInf,
                      "nums_left are actual nums not inf");
      append_array(loser_tree.template GetOne());
    }
    while (nums_left.back() == kInf) {  // merge almost ended
      nums_left.pop_back();
    }
    reset_amounts_of_slots_from();
    if (slots.size() == 1) {
      external_memory_buffer.Write(slots.front());
    } else {
      std::ranges::sort(slots, &SlotLess<keyCount>);
      SlotView current = slots.front();
      for (int idx = 1; idx < std::ssize(slots); idx++) {
        if (SlotEqual<keyCount>(slots[idx - 1], slots[idx])) {
          GetAggregateValue(current) += GetAggregateValue(slots[idx]);
        } else {
          external_memory_buffer.Write(current);
          current = slots[idx];
        }
      }
      external_memory_buffer.Write(current);
    }
  }
  external_memory_buffer.Flush();
  return external_memory_buffer.TotalWrites();
}
}  // namespace pmerge::ydb

#endif  // MERGE_SPILLING_BLOCKS_HPP
