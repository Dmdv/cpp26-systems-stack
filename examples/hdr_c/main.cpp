// Example: portable HDR vs optional HdrHistogram_c.
#include "ll/hdr_c.hpp"
#include "ll/tsc_clock.hpp"

#include <cstdio>

int main() {
  ll::HdrLatencyHistogram<> portable;
  ll::HdrHistogramC c;

  volatile double sink = 0;
  for (int i = 0; i < 5000; ++i) {
    const auto t0 = ll::steady_ns();
    for (int k = 0; k < 20; ++k) {
      sink += k;
    }
    const auto dt = static_cast<std::int64_t>(ll::steady_ns() - t0);
    portable.record(static_cast<std::uint64_t>(dt));
    c.record(dt < 0 ? 0 : dt);
  }

  std::printf("portable p99=%llu max=%llu\n",
              static_cast<unsigned long long>(portable.value_at_percentile(99.0).value_or(0)),
              static_cast<unsigned long long>(portable.max()));
#if LL_HAS_HDRHISTOGRAM_C
  std::printf("hdr_c    p99=%lld max=%lld mean=%.1f valid=%d\n",
              static_cast<long long>(c.value_at_percentile(99.0).value_or(0)),
              static_cast<long long>(c.max()), c.mean(), c.valid() ? 1 : 0);
#else
  std::printf("hdr_c    not linked (LL_HAS_HDRHISTOGRAM_C=0) — portable path still active\n");
#endif
  std::printf("sink=%f\n", sink);
  return 0;
}
