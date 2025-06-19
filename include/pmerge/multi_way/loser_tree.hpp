//
// Created by nfrmtk on 3/15/25.
//

#ifndef LOSER_TREE_HPP
#define LOSER_TREE_HPP
#include <immintrin.h>

#include <bit>
#include <cstdint>
#include <pmerge/common/resource.hpp>
#include <pmerge/two_way/simd_two_way.hpp>
#include <span>
#include <vector>
namespace pmerge::multi_way {
namespace detail {
template <typename T>
concept RandomAccessResourceIterator =
    std::ranges::random_access_range<T> &&
    pmerge::Resource<std::ranges::range_value_t<T>>;

template <pmerge::Resource Resource, size_t Num>
struct LoserTreeImpl {
  using Type = pmerge::two_way::SimdTwoWayMerger<
      typename LoserTreeImpl<Resource, Num / 2>::Type,
      typename LoserTreeImpl<Resource, Num - Num / 2>::Type>;
  template <typename Iter>
  static Type Make(Iter it) {
    static_assert(RandomAccessResourceIterator<Iter>);
    static_assert(Num >= 2);
    return Type{LoserTreeImpl<Resource, Num / 2>::Make(it),
                LoserTreeImpl<Resource, Num - Num / 2>::Make(it + Num / 2)};
  }
};

template <pmerge::Resource Resource>
struct LoserTreeImpl<Resource, 1> {
  using Type = Resource;
  template <typename Iter>
  static Type Make(Iter value) {
    static_assert(RandomAccessResourceIterator<Iter>);
    return Type{*value};
  }
};

}  // namespace detail

template <pmerge::Resource Resource, size_t ResourcesAmount>
using LoserTree =
    typename detail::LoserTreeImpl<Resource, ResourcesAmount>::Type;

template <size_t ResourcesAmount,
          std::ranges::random_access_range ResourceRange>
LoserTree<std::ranges::range_value_t<ResourceRange>, ResourcesAmount>
MakeLoserTree(ResourceRange&& range) {
  static_assert(detail::RandomAccessResourceIterator<ResourceRange>,
                "ResourceRange is range over resources");
  using ResourceType = std::ranges::range_value_t<ResourceRange>;
  return detail::LoserTreeImpl<ResourceType, ResourcesAmount>::Make(
      range.begin());
}

}  // namespace pmerge::multi_way
#endif  // LOSER_TREE_HPP
