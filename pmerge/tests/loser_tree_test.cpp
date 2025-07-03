//
// Created by nfrmtk on 3/16/25.
//
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <pmerge/common/vector_resource.hpp>
#include <pmerge/multi_way/loser_tree.hpp>
#include <ranges>
#include <vector>

#include "gtest/gtest.h"
#include "pmerge/common/print.hpp"
#include "pmerge/common/resource.hpp"
#include "pmerge/simd/utils.hpp"
#include "utils.hpp"
using pmerge::common::FromDataAndIndex;

template <typename Number>
class LoserTreeSuite : public UnmuteOnExitSuite {};

using test_depths = ::testing::Types<std::integral_constant<std::size_t, 1>,
                                     std::integral_constant<std::size_t, 2>,
                                     std::integral_constant<std::size_t, 3>,
                                     std::integral_constant<std::size_t, 4>>;
template <size_t ResourcesAmount>
void LoserTreeTest(const std::array<std::vector<pmerge::IntermediateInteger>,
                                    ResourcesAmount>& vectors) {
  auto result = SimpleMultiwayMerge(vectors);
  auto tree{pmerge::multi_way::MakeLoserTree<pmerge::common::VectorResource,
                                             ResourcesAmount>(vectors)};

  TestResource(tree, result);
}

TYPED_TEST_SUITE(LoserTreeSuite, test_depths);

TYPED_TEST(LoserTreeSuite, same_numbers) {
  constexpr int depth = TypeParam::value;
  using TreeType =
      pmerge::multi_way::LoserTree<pmerge::common::VectorResource, 1 << depth>;
  std::array<std::vector<pmerge::IntermediateInteger>, 1 << depth> data = {};
  for (size_t k = 0; auto& vec : data) {
    vec = pmerge::common::FromDataAndIndex(
        std::views::repeat(0, 16) | std::ranges::to<std::vector<uint64_t>>(),
        std::bitset<depth>{k++});
  }
  LoserTreeTest(data);
}

TYPED_TEST(LoserTreeSuite, inc_by_one) {
  pmerge::output.Mute();
  constexpr int depth = TypeParam::value;
  using TreeType =
      pmerge::multi_way::LoserTree<pmerge::common::VectorResource, 1 << depth>;
  std::array<std::vector<pmerge::IntermediateInteger>, 1 << depth> data = {};
  auto gen = MakeRandomGenerator(0, (1 << depth) - 1);
  for (size_t num : std::views::iota(0, 500)) {
    size_t index = gen();
    data[index].push_back(
        pmerge::PackFrom(num << (depth + 1), std::bitset<depth>{index}));
  }
  LoserTreeTest(data);
}

TEST(LoserTree, SimpleDepthTwo) {
  using TreeDepthTwo =
      pmerge::multi_way::LoserTree<pmerge::common::VectorResource, 4>;
  std::vector<std::vector<int64_t>> vectors;
  vectors.emplace_back(
      FromDataAndIndex(std::vector<uint64_t>{1, 2, 3, 4}, std::bitset<2>(0)));
  vectors.emplace_back(
      FromDataAndIndex(std::vector<uint64_t>{5, 5, 6, 6}, std::bitset<2>(1)));
  vectors.emplace_back(FromDataAndIndex(std::vector<uint64_t>{7, 10, 13, 54},
                                        std::bitset<2>(2)));
  vectors.emplace_back(
      FromDataAndIndex(std::vector<uint64_t>{1, 2, 3, 4}, std::bitset<2>(3)));

  auto result_simple = SimpleMultiwayMerge(vectors);
  TreeDepthTwo tree =
      pmerge::multi_way::MakeLoserTree<pmerge::common::VectorResource, 4>(
          vectors);
  TestResource(tree, result_simple);
}
