#include "utils.hpp"

class UnmuteOnExitSuite : public ::testing::Test {
  void SetUp() override {
    if (ForceMuteStdout()) {
      pmerge::output.Mute();
    }
  }
  void TearDown() override { pmerge::output.Unmute(); }
};
