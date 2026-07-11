// Example: SPSC handoff between a feed-like producer and a strategy-like consumer.
#include "ll/spsc_queue.hpp"
#include "ll/tsc_clock.hpp"

#include <cstdint>
#include <cstdio>
#include <thread>

struct Tick {
  std::uint64_t ts;
  double px;
  double qty;
};

int main() {
  ll::SpscQueue<Tick, 4096> q;
  constexpr int kN = 50'000;
  std::uint64_t checksum = 0;

  std::thread feed([&] {
    for (int i = 0; i < kN; ++i) {
      Tick t{ll::steady_ns(), 100.0 + (i % 10) * 0.01, 1.0};
      while (!q.try_push(t)) {
      }
    }
  });

  std::thread strat([&] {
    int got = 0;
    while (got < kN) {
      if (auto t = q.try_pop()) {
        checksum += static_cast<std::uint64_t>(t->px * 100) +
                    static_cast<std::uint64_t>(t->qty);
        ++got;
      }
    }
  });

  feed.join();
  strat.join();
  std::printf("spsc example ok  ticks=%d checksum=%llu\n", kN,
              static_cast<unsigned long long>(checksum));
  return 0;
}
