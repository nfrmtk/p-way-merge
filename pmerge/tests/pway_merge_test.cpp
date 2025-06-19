//
// Created by nfrmtk on 6/18/25.
//
#include <gtest/gtest.h>
#include <pmerge/ydb/merge_reference.hpp>
#include <pmerge/ydb/merge_spilling_blocks.hpp>

namespace simd {
using pmerge::ydb::merge2pway;
}

namespace scalar_reference {
using ydb_reference::merge2pway;
}

TSpillingBlock Generate(TSpilling& stats, int size_slots) {

}



TEST(merge2pway, Small) {

}