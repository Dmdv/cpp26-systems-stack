#pragma once

#include <chrono>
#include <cstdint>

#if defined(__x86_64__) || defined(_M_X64)
#include <x86intrin.h>
#endif

namespace ll {

// Low-overhead timestamps for microbenchmarks.
// - On x86_64: RDTSC via __rdtsc() (cycle counter; not invariant TSC-safe across cores
//   unless the platform guarantees it — pin the thread when comparing).
// - Elsewhere: steady_clock nanoseconds (macOS arm64, etc.).
//
// See docs/blueprint/06-telemetry.md
[[nodiscard]] inline std::uint64_t read_tsc() noexcept {
#if defined(__x86_64__) || defined(_M_X64)
  return static_cast<std::uint64_t>(__rdtsc());
#elif defined(__aarch64__)
  std::uint64_t v = 0;
  asm volatile("mrs %0, cntvct_el0" : "=r"(v));
  return v;
#else
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
#endif
}

[[nodiscard]] inline std::uint64_t steady_ns() noexcept {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

// Fixed-capacity latency sample buffer (no allocation after construction).
// Records raw deltas; percentiles computed offline (qsort or HDR later).
template <std::size_t Cap>
class LatencyBuffer {
 public:
  bool try_push(std::uint64_t sample_ns) noexcept {
    if (n_ >= Cap) {
      ++dropped_;
      return false;
    }
    data_[n_++] = sample_ns;
    return true;
  }

  [[nodiscard]] std::size_t size() const noexcept { return n_; }
  [[nodiscard]] std::size_t dropped() const noexcept { return dropped_; }
  [[nodiscard]] const std::uint64_t* data() const noexcept { return data_; }

  void clear() noexcept {
    n_ = 0;
    dropped_ = 0;
  }

 private:
  std::uint64_t data_[Cap]{};
  std::size_t n_{0};
  std::size_t dropped_{0};
};

}  // namespace ll
