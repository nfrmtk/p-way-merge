//
// Created by nfrmtk on 6/14/25.
//
#include <format>
#include <pmerge/simd/utils.hpp>
std::string pmerge::simd::ToString(__m256i reg) {
  return std::format("({}, {}, {}, {})",
                     pmerge::MakeReadableString(_mm256_extract_epi64(reg, 0)),
                     pmerge::MakeReadableString(_mm256_extract_epi64(reg, 1)),
                     pmerge::MakeReadableString(_mm256_extract_epi64(reg, 2)),
                     pmerge::MakeReadableString(_mm256_extract_epi64(reg, 3)));
}
std::string pmerge::ToStringHex(int64_t num) {
  return std::format("{:x}", num);
}
std::string pmerge::simd::ToHexString(__m256i reg) {
  return std::format("(0x{:x}, 0x{:x}, 0x{:x}, 0x{:x})",
                     GetAtPos<size_t, 0>(reg), GetAtPos<size_t, 1>(reg),
                     GetAtPos<size_t, 2>(reg), GetAtPos<size_t, 3>(reg));
}