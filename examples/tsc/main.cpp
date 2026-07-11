// Example: cycle/ns timestamps around a tight compute kernel.
#include "ll/tsc_clock.hpp"

#include <cstdio>

static double work(int n) {
  double s = 0;
  for (int i = 0; i < n; ++i) {
    s += static_cast<double>(i) * 0.5;
  }
  return s;
}

int main() {
  const auto c0 = ll::read_tsc();
  const auto n0 = ll::steady_ns();
  volatile double sink = work(100000);
  const auto c1 = ll::read_tsc();
  const auto n1 = ll::steady_ns();
  std::printf("tsc_delta=%llu ns_delta=%llu sink=%.1f\n",
              static_cast<unsigned long long>(c1 - c0),
              static_cast<unsigned long long>(n1 - n0), sink);
  return 0;
}
