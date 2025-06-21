//
// Created by nfrmtk on 4/16/25.
//

#ifndef UTILS_HPP
#define UTILS_HPP

#include <immintrin.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <ostream>
#include <random>
#include <vector>
#include <pmerge/ydb/spilling_mem.h>
#include <pmerge/simd/utils.hpp>
namespace std {
inline std::ostream& operator<<(std::ostream& os, __m256i reg) {
  os << pmerge::simd::ToString(reg);
  return os;
}
}  // namespace std

TSpillingBlock Make(TSpilling& stats, std::mt19937_64& gen, uint64_t size_blocks){
  
  auto storage = std::make_unique<uint64_t[]>(size_blocks);
  std::uniform_int_distribution<uint64_t> keys_distr()
  for()
}


template<ui32 keyCount>
bool Equal(std::span<const pmerge::ydb::Slot, keyCount>first, std::span<const pmerge::ydb::Slot, keyCount>second ) {
  for (int idx = 0; idx < keyCount; ++idx) {
    if (first[idx] != second[idx]) {
      return false;
    }
  }
  return true;
}

template<ui32 keyCount>
bool Less(std::span<const pmerge::ydb::Slot, keyCount>first, std::span<const pmerge::ydb::Slot, keyCount>second ) {
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

// it is transitive: for all a, b and c, if r(a, b) and r(b, c) are both true then r(a, c) is true;
// a = (2, 123, 4)
// b = (1, 3, 7)
// c = (1, 4, 312)
// a > c > b 

template<ui32 keyCount>
bool SlotLess(const pmerge::ydb::Slot& first, const pmerge::ydb::Slot& second) {
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
  if (Less(first_span, second_span) ) {
    return true;
  }else {
    PMERGE_ASSERT(std::ranges::lexicographical_compare(second_span, first_span));
    return false;
  }
}



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
    const std::vector<std::vector<int64_t>>& nums) {
  std::vector<int64_t> output;
  std::vector<int64_t> tmp;
  for (auto& vec : nums) {
    std::merge(vec.begin(), vec.end(), tmp.begin(), tmp.end(),
               std::back_inserter(output));
    tmp = std::move(output);
  }
  return tmp;
}
#endif  // UTILS_HPP
