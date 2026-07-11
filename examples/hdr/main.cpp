// Example: tail-latency tracking with ll::HdrLatencyHistogram.
#include "ll/hdr_histogram.hpp"
#include "ll/tsc_clock.hpp"

#include <cstdio>

int main() {
  ll::HdrLatencyHistogram<> hist;
  volatile double sink = 0;
  for (int i = 0; i < 10000; ++i) {
    const auto t0 = ll::steady_ns();
    for (int k = 0; k < 50; ++k) {
      sink += k * 0.001;
    }
    const auto dt = ll::steady_ns() - t0;
    hist.record(dt);
  }
  const auto p50 = hist.value_at_percentile(50.0).value_or(0);
  const auto p99 = hist.value_at_percentile(99.0).value_or(0);
  const auto p999 = hist.value_at_percentile(99.9).value_or(0);
  std::printf("samples=%llu mean=%.1f p50=%llu p99=%llu p99.9=%llu max=%llu ns\n",
              static_cast<unsigned long long>(hist.count()), hist.mean(),
              static_cast<unsigned long long>(p50),
              static_cast<unsigned long long>(p99),
              static_cast<unsigned long long>(p999),
              static_cast<unsigned long long>(hist.max()));
  std::printf("sink=%f\n", sink);
  return 0;
}
