//
// Created by nfrmtk on 3/16/25.
//
#include <gtest/gtest.h>

#include <pmerge/common/vector_resource.hpp>
#include <pmerge/multi_way/loser_tree.hpp>
#include <ranges>

#include "pmerge/simd/utils.hpp"
#include "utils.hpp"
template <size_t IndexSize>
std::vector<pmerge::IntermediateInteger> MakeInts(
    const std::vector<uint64_t>& data, std::bitset<IndexSize> index) {
  std::vector<pmerge::IntermediateInteger> result;
  for (uint64_t num : data) {
    result.push_back(pmerge::PackFrom(num, index));
  }
  return result;
}

TEST(LoserTree, Simple) {
  using TreeDepthTwo =
      pmerge::multi_way::LoserTree<pmerge::common::VectorResource, 4>;
  std::vector<std::vector<int64_t>> vectors;
  vectors.emplace_back(
      MakeInts(std::vector<uint64_t>{1, 2, 3, 4}, std::bitset<2>(0)));
  vectors.emplace_back(
      MakeInts(std::vector<uint64_t>{5, 5, 6, 6}, std::bitset<2>(1)));
  vectors.emplace_back(
      MakeInts(std::vector<uint64_t>{7, 10, 13, 54}, std::bitset<2>(2)));
  vectors.emplace_back(
      MakeInts(std::vector<uint64_t>{1, 2, 3, 4}, std::bitset<2>(3)));

  auto result_simple = SimpleMultiwayMerge(vectors);
  std::vector<pmerge::common::VectorResource> resources;
  resources.emplace_back(vectors[0]);
  resources.emplace_back(vectors[1]);
  resources.emplace_back(vectors[2]);
  resources.emplace_back(vectors[3]);
  TreeDepthTwo tree = pmerge::multi_way::MakeLoserTree<4>(resources.begin());
  std::vector<int64_t> result_tree;
  while (tree.Peek() != pmerge::kInf) {
    auto arr = pmerge::simd::AsArray(tree.GetOne());
    result_tree.push_back(arr[0]);
    result_tree.push_back(arr[1]);
    result_tree.push_back(arr[2]);
    result_tree.push_back(arr[3]);
  }
  while (result_tree.back() == pmerge::kInf) {
    result_tree.pop_back();
  }
  ASSERT_EQ(result_tree, result_simple);
}