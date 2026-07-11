#pragma once

// Low-overhead latency histogram emphasizing tail percentiles (p99 / p99.9 / max).
// Portable log2-bucket design — no heap traffic after construction.
// For production HFT desks, also consider HdrHistogram_c (see docs).
//
// See docs/blueprint/06-telemetry.md

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>

namespace ll {

// Records non-negative integer samples (e.g. nanoseconds or cycles).
// Buckets are power-of-two ranges: [0], [1], [2-3], [4-7], … up to 2^(Buckets-2)+.
template <std::size_t Buckets = 64>
class HdrLatencyHistogram {
 public:
  static_assert(Buckets >= 8 && Buckets <= 128);

  void record(std::uint64_t value) noexcept {
    ++count_;
    min_ = std::min(min_, value);
    max_ = std::max(max_, value);
    sum_ += value;
    const auto idx = bucket_index(value);
    ++buckets_[idx];
  }

  [[nodiscard]] std::uint64_t count() const noexcept { return count_; }
  [[nodiscard]] std::uint64_t min() const noexcept {
    return count_ ? min_ : 0;
  }
  [[nodiscard]] std::uint64_t max() const noexcept { return max_; }
  [[nodiscard]] double mean() const noexcept {
    return count_ ? static_cast<double>(sum_) / static_cast<double>(count_) : 0.0;
  }

  // percentile in (0, 100], e.g. 99.0, 99.9
  [[nodiscard]] std::optional<std::uint64_t> value_at_percentile(double pct) const noexcept {
    if (count_ == 0 || pct <= 0.0) {
      return std::nullopt;
    }
    const double clamped = std::min(pct, 100.0);
    const auto target =
        static_cast<std::uint64_t>(std::ceil(clamped / 100.0 * static_cast<double>(count_)));
    std::uint64_t seen = 0;
    for (std::size_t i = 0; i < Buckets; ++i) {
      seen += buckets_[i];
      if (seen >= target) {
        return bucket_representative(i);
      }
    }
    return max_;
  }

  void reset() noexcept {
    buckets_.fill(0);
    count_ = 0;
    sum_ = 0;
    min_ = std::numeric_limits<std::uint64_t>::max();
    max_ = 0;
  }

 private:
  static std::size_t bucket_index(std::uint64_t v) noexcept {
    if (v == 0) {
      return 0;
    }
    // floor(log2(v)) + 1, clamped
    const unsigned lz = static_cast<unsigned>(__builtin_clzll(v));
    const std::size_t log2 = 63u - lz;
    const std::size_t idx = log2 + 1;
    return idx < Buckets ? idx : Buckets - 1;
  }

  static std::uint64_t bucket_representative(std::size_t idx) noexcept {
    if (idx == 0) {
      return 0;
    }
    // geometric mid of [2^(idx-1), 2^idx)
    const std::uint64_t lo = 1ull << (idx - 1);
    return lo + (lo / 2);
  }

  std::array<std::uint64_t, Buckets> buckets_{};
  std::uint64_t count_{0};
  std::uint64_t sum_{0};
  std::uint64_t min_{std::numeric_limits<std::uint64_t>::max()};
  std::uint64_t max_{0};
};

}  // namespace ll
