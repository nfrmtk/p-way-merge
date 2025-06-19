//
// Created by nfrmtk on 4/9/25.
//
#include <gtest/gtest.h>

#include <pmerge/simd/inregister_merge.hpp>
#include <pmerge/simd/utils.hpp>
#include <pmerge/two_way/simd_two_way.hpp>
#include <random>

#include "utils.hpp"

TEST(Simd, MergeSimple) {
  auto reg_odd = pmerge::simd::MakeFrom(1, 3, 5, 7);
  auto reg_even = pmerge::simd::MakeFrom(2, 4, 6, 8);
  PMERGE_MERGE(reg_even, reg_odd);
  ASSERT_PRED2(CompareRegistersEqual, reg_even, MakeFrom(1, 2, 3, 4));
  ASSERT_PRED2(CompareRegistersEqual, reg_odd, MakeFrom(5, 6, 7, 8));
}

TEST(Simd, CompleteEnumerationNoEqualElements) {
  ForEachPermutationsOfRegisters([](__m256i vMin, __m256i vMax) {
    PMERGE_MERGE(vMin, vMax);
    ASSERT_PRED2(CompareRegistersEqual, vMin, MakeFrom(1, 2, 3, 4));
    ASSERT_PRED2(CompareRegistersEqual, vMax, MakeFrom(5, 6, 7, 8));
  });
}

TEST(Simd, CompleteEnumerationNoEqualElementsButBig) {
  ForEachPermutationsOfRegisters([](__m256i vMin, __m256i vMax) {
    auto mask = pmerge::simd::MakeFrom(0, 0, pmerge::kInf, pmerge::kInf);
    PMERGE_MINMAX(mask, vMin);
    mask = pmerge::simd::MakeFrom(0, 0, 0, pmerge::kInf);
    PMERGE_MINMAX(mask, vMax);
    auto ans = MergeSimple(vMin, vMax);
    PMERGE_MERGE(vMin, vMax);
    ASSERT_PRED2(CompareRegistersEqual, vMin,
                 pmerge::simd::MakeFrom(ans[0], ans[1], ans[2], ans[3]));
    ASSERT_PRED2(CompareRegistersEqual, vMax,
                 pmerge::simd::MakeFrom(ans[4], ans[5], ans[6], ans[7]));
  });
}

TEST(Simd, MinMax) {
  auto reg_bigger = MakeFrom(1, 1, 4, 4);
  auto reg_smaller = MakeFrom(3, 3, 2, 2);

  pmerge::simd::MinMax(reg_smaller, reg_bigger);
  ASSERT_PRED2(CompareRegistersEqual, reg_smaller, MakeFrom(1, 1, 2, 2))
      << ToString(reg_bigger);
  ASSERT_PRED2(CompareRegistersEqual, reg_bigger, MakeFrom(3, 3, 4, 4))
      << ToString(reg_bigger);
}

TEST(Simd, GetSingleUInt64) {
  auto reg = MakeFrom(1, 2, 3, 4);
  ASSERT_EQ(pmerge::simd::Get64MostSignificantBits(reg), 1);
}
