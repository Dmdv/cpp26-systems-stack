#include <gtest/gtest.h>
#include <fmt/format.h>

// Minimal GTest target so googletest's Homebrew install is verified.
TEST(GoogletestSmoke, FormatsWithFmt) {
  EXPECT_EQ(fmt::format("g{}", 26), "g26");
}

TEST(GoogletestSmoke, Arithmetic) {
  EXPECT_EQ(2 + 2, 4);
}
