#include "folly_compat.hpp"
#include <catch2/catch_test_macros.hpp>

#include <folly/FBString.h>
#include <folly/Optional.h>
#include <folly/String.h>
#include <folly/container/F14Map.h>
#include <folly/futures/Future.h>

#include <string>
#include <vector>

TEST_CASE("folly fbstring append", "[folly]") {
  folly::fbstring s("lib");
  s.append("_smoke");
  REQUIRE(s == "lib_smoke");
  REQUIRE(s.size() == 9);
}

TEST_CASE("folly Optional", "[folly]") {
  folly::Optional<std::string> empty;
  REQUIRE_FALSE(empty.has_value());
  folly::Optional<std::string> full("ok");
  REQUIRE(full.has_value());
  REQUIRE(*full == "ok");
}

TEST_CASE("folly F14FastMap", "[folly]") {
  folly::F14FastMap<int, int> m;
  m.emplace(1, 10);
  m.emplace(2, 20);
  REQUIRE(m.at(1) == 10);
  REQUIRE(m.find(2) != m.end());
}

TEST_CASE("folly join", "[folly]") {
  const std::vector<std::string> parts{"a", "b", "c"};
  REQUIRE(folly::join("-", parts) == "a-b-c");
}

TEST_CASE("folly Future then", "[folly]") {
  auto f = folly::makeFuture(21).thenValue([](int v) { return v * 2; });
  REQUIRE(std::move(f).get() == 42);
}
