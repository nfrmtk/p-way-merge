#pragma once
#include <pmerge/fs/records_source.hpp>
#include <vector>
namespace pmerge::multi_way {
class LinearMultiWayMerger {
 public:
  LinearMultiWayMerger();
  void Merge(const std::vector<pmerge::fs::RecordsSource>& sources,
             const std::filesystem::path& out);
};
}  // namespace pmerge::multi_way
