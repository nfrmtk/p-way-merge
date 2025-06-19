//
// Created by nfrmtk on 4/9/25.
//

#ifndef INREGISTER_MERGE_HPP
#define INREGISTER_MERGE_HPP
#include <immintrin.h>

#include <cstdint>
#define PMERGE_FORCE_INLINE \
  __attribute__((always_inline))  // todo: research forceinline functions
                                  // instead of macros

#define PMERGE_MINMAX(vMin, vMax)                     \
  {                                                   \
    auto mask = _mm256_cmpgt_epi64(vMin, vMax);       \
    auto vTmp = _mm256_blendv_epi8(vMin, vMax, mask); \
    vMax = _mm256_blendv_epi8(vMax, vMin, mask);      \
    vMin = vTmp;                                      \
  }

#define PMERGE_MERGE(vMin, vMax)                                    \
  {                                                                 \
    PMERGE_MINMAX(vMin, vMax)                                       \
    vMin = _mm256_alignr_epi8(vMin, vMin, 8);                       \
    PMERGE_MINMAX(vMin, vMax)                                       \
    vMin = _mm256_permute4x64_epi64(vMin, _MM_SHUFFLE(0, 1, 2, 3)); \
    PMERGE_MINMAX(vMin, vMax)                                       \
    vMin = _mm256_alignr_epi8(vMin, vMin, 8);                       \
    PMERGE_MINMAX(vMin, vMax)                                       \
    vMin = _mm256_permute4x64_epi64(vMin, _MM_SHUFFLE(0, 1, 2, 3)); \
  }

#endif  // INREGISTER_MERGE_HPP
