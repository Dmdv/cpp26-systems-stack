#include <catch2/catch_test_macros.hpp>

#include <taskflow/taskflow.hpp>

#include <atomic>
#include <mutex>
#include <vector>

TEST_CASE("Taskflow linear dependency graph", "[taskflow]") {
  tf::Executor executor;
  tf::Taskflow flow;

  std::vector<int> order;
  order.reserve(3);
  std::mutex mu;

  auto A = flow.emplace([&] {
    std::lock_guard lock(mu);
    order.push_back(1);
  });
  auto B = flow.emplace([&] {
    std::lock_guard lock(mu);
    order.push_back(2);
  });
  auto C = flow.emplace([&] {
    std::lock_guard lock(mu);
    order.push_back(3);
  });
  A.precede(B);
  B.precede(C);

  executor.run(flow).wait();
  REQUIRE(order == std::vector<int>{1, 2, 3});
}

TEST_CASE("Taskflow parallel independent tasks accumulate", "[taskflow]") {
  tf::Executor executor;
  tf::Taskflow flow;

  constexpr int N = 64;
  std::atomic<long long> sum{0};
  for (int i = 1; i <= N; ++i) {
    flow.emplace([&, i] {
      sum.fetch_add(i, std::memory_order_relaxed);
    });
  }

  executor.run(flow).wait();
  REQUIRE(sum.load() == static_cast<long long>(N) * (N + 1) / 2);
}

TEST_CASE("Taskflow condition task branches", "[taskflow]") {
  tf::Executor executor;
  tf::Taskflow flow;

  std::atomic<int> path{-1};
  auto init = flow.emplace([] {});
  auto cond = flow.emplace([] { return 1; });  // take second branch
  auto left = flow.emplace([&] { path.store(0); });
  auto right = flow.emplace([&] { path.store(1); });

  init.precede(cond);
  cond.precede(left, right);

  executor.run(flow).wait();
  REQUIRE(path.load() == 1);
}
