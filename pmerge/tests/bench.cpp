#include <pmerge/ydb/spilling_mem.h>

#include <cstddef>
#include <limits>
#include <pmerge/ydb/merge_spilling_blocks.hpp>

#include "pmerge/common/print.hpp"
#include "pmerge/ydb/merge_reference.hpp"
#include "utils.hpp"

int main() {
  auto kind = DetectMergeKind();
  pmerge::output.Mute();
  if (!kind.has_value()) {
    std::cout << std::format("couldn't detect merge kind") << std::endl;
    return 2;
  }
  TSpilling stats{8 * 1024 * 1024};
  auto rng_keys = MakeRandomGenerator(0, std::numeric_limits<size_t>::max());
  int low = 1 << 19;
  auto rng_sizes = MakeRandomGenerator(low, low * 3 - 1);
  std::vector<uint64_t> buff;
  constexpr int kBuffSizeSlots = 96 * 8;
  buff.resize(kBuffSizeSlots * 8);

  auto keys_gen = [&rng_keys] { return rng_keys(); };
  auto counts_gen = [] { return 1; };
  auto sizes_gen = [&rng_sizes] { return rng_sizes(); };
  auto deque =
      MakeSpillBlocksDeque<2, 4>(stats, keys_gen, counts_gen, sizes_gen);
  size_t writes = 0;
  switch (*kind) {
    case MultiwayMergeKind::kReference:
      writes = ydb_reference::merge2pway<false, 2, 4>(
          buff.data(), kBuffSizeSlots, stats, deque);
      break;
    case MultiwayMergeKind::kSimdLoserTree:
      writes = pmerge::ydb::merge2pway<false, 2, 4>(buff.data(), kBuffSizeSlots,
                                                    stats, deque);
      break;
  }
  std::cout << std::format("finihed: total writes: {}\n", writes);
}