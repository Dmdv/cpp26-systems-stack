#include <catch2/catch_test_macros.hpp>

#include "ll/affinity.hpp"
#include "ll/arena.hpp"
#include "ll/branch.hpp"
#include "ll/cache_line.hpp"
#include "ll/spsc_queue.hpp"
#include "ll/tsc_clock.hpp"

#include <cstdint>
#include <string>
#include <thread>
#include <vector>

TEST_CASE("cache line constant is power-of-two and large enough", "[ll][cache]") {
  REQUIRE(ll::kCacheLine >= 64);
  REQUIRE((ll::kCacheLine & (ll::kCacheLine - 1)) == 0);
}

TEST_CASE("spsc queue push/pop and full/empty", "[ll][spsc]") {
  ll::SpscQueue<int, 8> q;
  REQUIRE(q.empty());
  REQUIRE(q.try_push(1));
  REQUIRE(q.try_push(2));
  auto a = q.try_pop();
  auto b = q.try_pop();
  REQUIRE(a.has_value());
  REQUIRE(b.has_value());
  REQUIRE(*a == 1);
  REQUIRE(*b == 2);
  REQUIRE_FALSE(q.try_pop().has_value());
}

TEST_CASE("spsc queue stress single producer consumer", "[ll][spsc]") {
  constexpr int N = 100000;
  ll::SpscQueue<int, 1024> q;
  std::int64_t sum = 0;
  std::thread prod([&] {
    for (int i = 1; i <= N; ++i) {
      while (!q.try_push(i)) {
      }
    }
  });
  std::thread cons([&] {
    int got = 0;
    while (got < N) {
      if (auto v = q.try_pop()) {
        sum += *v;
        ++got;
      }
    }
  });
  prod.join();
  cons.join();
  REQUIRE(sum == static_cast<std::int64_t>(N) * (N + 1) / 2);
}

TEST_CASE("arena bump allocate and reset", "[ll][arena]") {
  ll::Arena arena(4096);
  auto* a = arena.create<int>(42);
  auto* b = arena.create<double>(1.5);
  REQUIRE(*a == 42);
  REQUIRE(*b == 1.5);
  REQUIRE(arena.used() > 0);
  arena.reset();
  REQUIRE(arena.used() == 0);
  auto* c = arena.create<int>(7);
  REQUIRE(*c == 7);
}

TEST_CASE("object pool acquire/release", "[ll][pool]") {
  ll::ObjectPool<int, 4> pool;
  REQUIRE(pool.available() == 4);
  int* p = pool.acquire();
  REQUIRE(p != nullptr);
  REQUIRE(pool.available() == 3);
  *p = 9;
  pool.release(p);
  REQUIRE(pool.available() == 4);
}

TEST_CASE("tsc and steady clocks move forward", "[ll][tsc]") {
  const auto t0 = ll::read_tsc();
  const auto n0 = ll::steady_ns();
  volatile int x = 0;
  for (int i = 0; i < 1000; ++i) {
    x += i;
  }
  REQUIRE(ll::read_tsc() >= t0);
  REQUIRE(ll::steady_ns() >= n0);
  ll::LatencyBuffer<16> buf;
  REQUIRE(buf.try_push(10));
  REQUIRE(buf.size() == 1);
}

TEST_CASE("affinity backend is non-empty", "[ll][affinity]") {
  REQUIRE(ll::affinity_backend() != nullptr);
  REQUIRE(std::string(ll::affinity_backend()).size() > 0);
  (void)ll::set_current_thread_interactive_qos();
}

TEST_CASE("static polymorphism crtp helper", "[ll][branch]") {
  struct Impl : ll::StaticInterface<Impl> {
    int value() const { return 11; }
  };
  Impl x;
  REQUIRE(x.self().value() == 11);
}
