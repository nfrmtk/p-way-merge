//
// Created by nfrmtk on 4/11/25.
//
#include <gtest/gtest.h>

#include <algorithm>
#include <pmerge/simd/utils.hpp>
#include <pmerge/two_way/simd_two_way.hpp>
#include <pmerge/ydb/spilling_block_resource.hpp>
#include <random>
#include <ranges>

#include "utils.hpp"
static constexpr int kSpillBlockSize = 1 << 10;
template <typename Value>
std::span<Value> AsSpan(TSpillingBlock& block) {
  return std::span{static_cast<const pmerge::ydb::Slot*>(block.ExternalMemory),
                   static_cast<const pmerge::ydb::Slot*>(block.ExternalMemory) +
                       block.BlockSize / sizeof(pmerge::ydb::Slot)};
}
template <typename T>
TSpillingBlock MakeSpillingBlock(const std::unique_ptr<T>& pointer,
                                 int64_t size) {
  return TSpillingBlock{pointer.get(),
                        size * sizeof(std::remove_all_extents_t<T>), 0};
}

constexpr int kExternalMemorySize = 1 << 15;

void TestMerge() {
  TSpilling stats{8 * 1024 * 1024};
  std::unique_ptr<uint64_t[]> first_external_memory =
      std::make_unique<uint64_t[]>(kExternalMemorySize);
  std::mt19937_64 gen(123);
  std::uniform_int_distribution<int64_t> distr(
      1, 12);  // a == 1 because i dont allow duplicates
  int starting_point = 0;
  std::generate_n(first_external_memory.get(), kExternalMemorySize,
                  [&]() { return starting_point += distr(gen); });
  starting_point = 0;
  std::unique_ptr<uint64_t[]> second_external_memory =
      std::make_unique<uint64_t[]>(kExternalMemorySize);
  std::generate_n(second_external_memory.get(), kExternalMemorySize,
                  [&]() { return starting_point += distr(gen); });
  starting_point = 0;

  static constexpr int kBuffSize = 1 << 8;

  std::unique_ptr<pmerge::ydb::Slot[]> first_buff =
      std::make_unique<pmerge::ydb::Slot[]>(kBuffSize);
  std::generate_n(first_buff.get(), kBuffSize,
                  []() { return pmerge::ydb::Slot{}; });
  std::unique_ptr<pmerge::ydb::Slot[]> second_buff =
      std::make_unique<pmerge::ydb::Slot[]>(kBuffSize);

  std::generate_n(second_buff.get(), kBuffSize,
                  []() { return pmerge::ydb::Slot{}; });
  auto first_spilling_block =
      MakeSpillingBlock(first_external_memory, kExternalMemorySize);
  auto second_spilling_block =
      MakeSpillingBlock(second_external_memory, kExternalMemorySize);
  pmerge::ydb::SpillingBlockBufferedResource<1> res1(
      stats, first_spilling_block,
      std::span<pmerge::ydb::Slot>{first_buff.get(),
                                   first_buff.get() + kBuffSize},
      std::bitset<1>{0});
  pmerge::ydb::SpillingBlockBufferedResource<1> res2(
      stats, second_spilling_block,
      std::span<pmerge::ydb::Slot>{second_buff.get(),
                                   second_buff.get() + kBuffSize},
      std::bitset<1>{1});

  std::vector<int64_t> merged;
  auto first_range = AsSpan<const pmerge::ydb::Slot>(first_spilling_block) |
                     std::views::transform([](const pmerge::ydb::Slot& slot) {
                       auto val =
                           pmerge::PackFrom(slot.data[0], std::bitset<1>{0});
                       return val;
                     });
  auto second_range =
      AsSpan<const pmerge::ydb::Slot>(second_spilling_block) |
      std::views::transform([](const pmerge::ydb::Slot& slot) {
        return pmerge::PackFrom(slot.data[0], std::bitset<1>{1});
      });
  auto get_real_num = std::views::transform(
      [](int64_t num) { return num - std::numeric_limits<int64_t>::min(); });
  std::ranges::merge(first_range, second_range, std::back_inserter(merged));
  std::cout << "total size: " << merged.size() << '\n';
  for (auto num : merged | get_real_num) {
    std::cout << num << " ";
  }
  std::cout << std::endl;

  for (auto num : first_range | get_real_num) {
    std::cout << num << ' ';
  }
  std::cout << std::endl;

  for (auto num : second_range | get_real_num) {
    std::cout << num << ' ';
  }
  std::cout << std::endl;

  // pmerge::common::VectorResource res2(second_vec);
  pmerge::two_way::SimdTwoWayMerger merger{std::move(res1), std::move(res2)};
  static_assert(pmerge::Resource<decltype(merger)>, "asd");

  ASSERT_EQ(merger.Peek(), merged.front())
      << std::hex << "merger: " << merger.Peek()
      << " merged: " << merged.front();

  for (int64_t idx = 0; idx + 4 <= std::ssize(merged); idx += 4) {
    auto reg = merger.GetOne();
    std::cout << std::format(
        "me: {}, stl: {}\n", pmerge::simd::ToString(reg),
        pmerge::simd::ToString(_mm256_loadu_si256(
            reinterpret_cast<const __m256i*>(merged.data() + idx))));
    auto arr = pmerge::simd::AsArray(reg);
    std::cout << idx << '\n';
    ASSERT_EQ(arr[0], merged[idx + 0])
        << std::format("{:x} {:x} {:x}\n", idx + 0, arr[0], merged[idx]);
    ASSERT_EQ(arr[1], merged[idx + 1])
        << std::format("{:x} {:x} {:x}\n", idx + 1, arr[1], merged[idx + 1]);
    ASSERT_EQ(arr[2], merged[idx + 2])
        << std::format("{:x} {:x} {:x}\n", idx + 2, arr[2], merged[idx + 2]);
    ASSERT_EQ(arr[3], merged[idx + 3])
        << std::format("{:x} {:x} {:x}\n", idx + 3, arr[3], merged[idx + 3]);
  }
}

TEST(Merge, Random) { TestMerge(); }
