//
// Created by nfrmtk on 4/16/25.
//

#ifndef UTILS_HPP
#define UTILS_HPP

#include <immintrin.h>

#include <ostream>
#include <random>
#include <vector>
namespace std {
inline std::ostream& operator<<(std::ostream& os, __m256i reg) {
  os << pmerge::simd::ToString(reg);
  return os;
}
}  // namespace std

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

inline void FillVector(std::vector<int64_t>& vec, std::mt19937& random) {
  int current = 0;
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
