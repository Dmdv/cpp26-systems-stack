#include <catch2/catch_test_macros.hpp>

#include <hpx/hpx_main.hpp>
#include <hpx/future.hpp>
#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>

#include <numeric>
#include <vector>

TEST_CASE("hpx ready future", "[hpx]") {
  auto f = hpx::make_ready_future(7);
  REQUIRE(f.get() == 7);
}

TEST_CASE("hpx future continuation", "[hpx]") {
  auto f = hpx::make_ready_future(3).then([](hpx::future<int> r) {
    return r.get() + 4;
  });
  REQUIRE(f.get() == 7);
}

TEST_CASE("hpx parallel reduce", "[hpx]") {
  std::vector<int> v(500);
  std::iota(v.begin(), v.end(), 1);
  const auto sum = hpx::reduce(hpx::execution::par, v.begin(), v.end(), 0LL);
  REQUIRE(sum == 500LL * 501 / 2);
}

TEST_CASE("hpx parallel for_each mutates", "[hpx]") {
  std::vector<int> v(64, 1);
  hpx::for_each(hpx::execution::par, v.begin(), v.end(), [](int& x) { x *= 2; });
  REQUIRE(std::accumulate(v.begin(), v.end(), 0) == 128);
}
