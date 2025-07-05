//
// Created by nfrmtk on 4/16/25.
//

#ifndef UTILS_HPP
#define UTILS_HPP

#include <gtest/gtest.h>
#include <immintrin.h>
#include <pmerge/ydb/spilling_mem.h>

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <memory>
#include <numeric>
#include <ostream>
#include <pmerge/common/print.hpp>
#include <pmerge/simd/utils.hpp>
#include <random>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

#include "pmerge/common/assert.hpp"
#include "pmerge/common/resource.hpp"
#include "pmerge/ydb/types.hpp"
namespace std {
inline std::ostream& operator<<(std::ostream& os, __m256i reg) {
  os << pmerge::simd::ToString(reg);
  return os;
}
}  // namespace std
namespace pmerge::ydb {

template <ui64 keyCount>
bool Equal(pmerge::ydb::Key<keyCount> first,
           pmerge::ydb::Key<keyCount> second) {
  for (int idx = 0; idx < keyCount; ++idx) {
    if (first[idx] != second[idx]) {
      return false;
    }
  }
  return true;
}

template <ui64 keyCount>
bool LexicographicallyLess(pmerge::ydb::Key<keyCount> first,
                           pmerge::ydb::Key<keyCount> second) {
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

template <auto keySize>
uint64_t Hash(pmerge::ydb::Key<keySize> key) {
  return std::accumulate(key.begin(), key.end(), 0ull);
}

template <uint32_t keySize>
TSpillingBlock MakeRandomSlotsBlock(TSpilling& stats, std::mt19937_64& gen,
                                    uint64_t size_slots) {
  const int64_t size_nums = size_slots * 8;
  std::uniform_int_distribution<uint64_t> nums(0);
  std::uniform_int_distribution<uint64_t> counts(0, 10);
  auto storage = std::make_unique<Slot[]>(size_slots);
  for (int idx = 0; idx < size_slots; idx++) {
    Slot* this_slot = storage.get() + idx;
    std::generate_n(&this_slot->nums[1], keySize, [&] { return nums(gen); });
    this_slot->nums[0] =
        Hash(pmerge::ydb::GetKey<keySize>(ConstSlotView{this_slot->nums}));
    this_slot->nums[7] = counts(gen);
  }
  std::sort(storage.get(), storage.get() + size_slots, SlotLess<keySize>);
  return stats.Save(storage.get(), size_slots * 64, 0);
}
template <uint32_t keySize>
TSpillingBlock MakeSlotsBlock(TSpilling& stats, auto keys_gen, auto counts_gen,
                              int64_t size_slots) {
  auto storage = std::make_unique<Slot[]>(size_slots);
  for (int idx = 0; idx < size_slots; idx++) {
    Slot* this_slot = storage.get() + idx;
    std::generate_n(&this_slot->nums[1], keySize, [&] { return keys_gen(); });
    this_slot->nums[0] =
        Hash(pmerge::ydb::GetKey<keySize>(ConstSlotView{this_slot->nums}));
    this_slot->nums[7] = counts_gen();
  }
  std::sort(storage.get(), storage.get() + size_slots, SlotLess<keySize>);
  return stats.Save(storage.get(), size_slots * 64, 0);
}

inline std::vector<Slot> AsSlotsVector(TSpilling& stats,
                                       const TSpillingBlock& block) {
  std::vector<Slot> slots;
  PMERGE_ASSERT(block.BlockSize % 64 == 0);
  slots.resize(block.BlockSize / 64);
  stats.Load(block, 0, slots.data(), block.BlockSize);
  return slots;
}

}  // namespace pmerge::ydb

template <size_t KeySize, size_t IndexBits>
auto ToIntermediateIntegers(std::bitset<IndexBits> index) {
  return std::views::transform([=](const pmerge::ydb::Slot& slot) {
           return pmerge::PackFrom(pmerge::ydb::GetHash(slot), index);
         }) |
         std::ranges::to<std::vector<pmerge::IntermediateInteger>>();
}

template <size_t KeySize, size_t IndexBits>
auto SpillBlockToInts(TSpilling& stats, const TSpillingBlock& block,
                      std::bitset<IndexBits> index) {
  return pmerge::ydb::AsSlotsVector(stats, block) |
         ToIntermediateIntegers<KeySize, IndexBits>(index);
}

inline std::vector<uint64_t> MakeBuffer(int size_slots) {
  return std::vector<uint64_t>(size_slots * 8, 0);
}

std::string RangeToString(std::ranges::range auto&& range) {
  std::string str;
  for (const auto& val : range) {
    str += std::format("{}", val);
  }
  return str;
}

void PrintIntermediateIntegersRange(std::ranges::range auto&& range) {
  for (const auto& num : range) {
    pmerge::utils::PrintIfDebug(pmerge::MakeReadableString(num) + ' ');
  }
  pmerge::output << std::endl;
}

template <typename T>
void F() = delete;

inline void GetComplimentary(const uint64_t* arr, uint64_t* dst) {
  int out_idx = 0;
  for (uint64_t num = 1; num <= 8; ++num) {
    auto p = std::find(arr, arr + 4, num);
    if (p == arr + 4) {
      dst[out_idx] = num;
      ++out_idx;
    }
  }
}

template <typename F>
class Defer {
 public:
  Defer(F&& f) : fun_(std::forward<F>(f)) {
    static_assert(std::is_nothrow_invocable_v<F>);
  }
  ~Defer() { fun_(); }

 private:
  F fun_;
};

template <typename Callback>
void ForEachPermutationsOfRegisters(Callback cb) {
  uint64_t vec_left[4] = {1, 2, 3, 4};
  while (true) {
    {
      uint64_t vec_right[4];
      GetComplimentary(vec_left, vec_right);
      auto vMin = pmerge::simd::MakeFrom(vec_left[0], vec_left[1], vec_left[2],
                                         vec_left[3]);
      auto vMax = pmerge::simd::MakeFrom(vec_right[0], vec_right[1],
                                         vec_right[2], vec_right[3]);
      cb(vMin, vMax);
    }
    int i = 3;  // Идем с конца
    while (i >= 0 && vec_left[i] >= 8 - (3 - i)) {
      --i;
    }
    if (i < 0) break;  // Если не можем увеличить — конец

    // Увеличиваем vec[i] и обновляем последующие элементы
    ++vec_left[i];
    for (int j = i + 1; j < 4; ++j) {
      vec_left[j] = vec_left[j - 1] + 1;
    }
  }
}

inline std::array<uint64_t, 8> MergeSimple(__m256i first, __m256i second) {
  auto first_arr = pmerge::simd::AsArray(first);
  auto second_arr = pmerge::simd::AsArray(second);

  std::array<uint64_t, 8> ret;
  std::merge(first_arr.begin(), first_arr.end(), second_arr.begin(),
             second_arr.end(), ret.begin());
  return ret;
}

inline void FillVector(std::vector<uint64_t>& vec, std::mt19937& random) {
  uint64_t current = 0;
  std::uniform_int_distribution<> distr(0, 10);
  for (auto& num : vec) {
    num = current += distr(random);
  }
}

inline std::vector<int64_t> SimpleMultiwayMerge(
    const std::ranges::range auto& nums)
  requires std::same_as<std::vector<int64_t>,
                        std::ranges::range_value_t<decltype(nums)>>
{
  std::vector<int64_t> output;
  std::vector<int64_t> tmp;
  for (auto& vec : nums) {
    std::merge(vec.begin(), vec.end(), tmp.begin(), tmp.end(),
               std::back_inserter(output));
    tmp = std::move(output);
  }
  return tmp;
}

inline std::vector<pmerge::ydb::Slot> SimpleMultiwayMerge(
    const std::ranges::range auto& nums)
  requires std::same_as<std::vector<pmerge::ydb::Slot>,
                        std::ranges::range_value_t<decltype(nums)>>
{
  std::vector<pmerge::ydb::Slot> output;
  std::vector<pmerge::ydb::Slot> tmp;
  for (int k = 0; auto& vec : nums) {
    std::merge(vec.begin(), vec.end(), tmp.begin(), tmp.end(),
               std::back_inserter(output));
    tmp = std::move(output);
  }
  return tmp;
}

inline auto MakeRandomGenerator(uint64_t low, uint64_t high,
                                uint64_t seed = 123) {
  struct Generator {
    std::mt19937_64 rng{seed};
    std::uniform_int_distribution<uint64_t> distr{low, high};
    uint64_t operator()() noexcept { return distr(rng); }
  };
  return Generator{};
}
class UnmuteOnExitSuite : public ::testing::Test {
  void TearDown() override { pmerge::output.Unmute(); }
};

std::vector<pmerge::IntermediateInteger> AsVector(
    pmerge::Resource auto& resource) {
  std::vector<pmerge::IntermediateInteger> ints;
  bool stopped = false;
  while (true) {
    auto arr = pmerge::simd::AsArray(resource.GetOne());
    for (int64_t num : arr) {
      if (num == pmerge::kInf) {
        stopped = true;
        break;
      }
      ints.push_back(num);
    }
    if (stopped) {
      break;
    }
  }
  return ints;
}

void TestResource(pmerge::Resource auto& tested_resouce,
                  std::ranges::random_access_range auto&& answer) {
  ASSERT_TRUE(std::ranges::is_sorted(answer)) << RangeToString(answer);
  std::vector<pmerge::IntermediateInteger> test_vector{
      AsVector(tested_resouce)};
  ASSERT_EQ(test_vector.size(), std::size(answer));
  for (int idx = 0; idx < std::ssize(test_vector); ++idx) {
    ASSERT_TRUE(pmerge::IsValid(test_vector[idx]));

    ASSERT_EQ(test_vector[idx], answer[idx])
        << std::format("got from resource: {}, vs expected: {}. ",
                       pmerge::MakeReadableString(test_vector[idx]),
                       pmerge::MakeReadableString(answer[idx]));
  }
}

#endif  // UTILS_HPP
