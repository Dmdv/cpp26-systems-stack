#pragma once

// Optional wrapper around HdrHistogram_c (when LL_HAS_HDRHISTOGRAM_C=1).
// Falls back to documenting unavailable state; portable path remains ll::HdrLatencyHistogram.
// See docs/tutorials/hdrhistogram-c.md

#include "ll/hdr_histogram.hpp"

#include <cstdint>
#include <optional>

#ifndef LL_HAS_HDRHISTOGRAM_C
#define LL_HAS_HDRHISTOGRAM_C 0
#endif

#if LL_HAS_HDRHISTOGRAM_C
#include <hdr/hdr_histogram.h>
#endif

namespace ll {

[[nodiscard]] inline bool hdr_c_available() noexcept {
#if LL_HAS_HDRHISTOGRAM_C
  return true;
#else
  return false;
#endif
}

// Production-grade HDR histogram (HdrHistogram_c) with value range [1, highest_trackable].
// significant_figures typically 3.
class HdrHistogramC {
 public:
#if LL_HAS_HDRHISTOGRAM_C
  explicit HdrHistogramC(std::int64_t highest_trackable = 3'600'000'000LL,
                         int significant_figures = 3) {
    // lowest trackable value = 1
    if (hdr_init(1, highest_trackable, significant_figures, &hist_) != 0) {
      hist_ = nullptr;
    }
  }

  HdrHistogramC(const HdrHistogramC&) = delete;
  HdrHistogramC& operator=(const HdrHistogramC&) = delete;

  ~HdrHistogramC() {
    if (hist_) {
      hdr_close(hist_);
      hist_ = nullptr;
    }
  }

  [[nodiscard]] bool valid() const noexcept { return hist_ != nullptr; }

  void record(std::int64_t value) noexcept {
    if (hist_ && value >= 0) {
      hdr_record_value(hist_, value);
    }
  }

  [[nodiscard]] std::int64_t min() const noexcept {
    return hist_ ? hdr_min(hist_) : 0;
  }
  [[nodiscard]] std::int64_t max() const noexcept {
    return hist_ ? hdr_max(hist_) : 0;
  }
  [[nodiscard]] double mean() const noexcept {
    return hist_ ? hdr_mean(hist_) : 0.0;
  }
  [[nodiscard]] std::optional<std::int64_t> value_at_percentile(double pct) const noexcept {
    if (!hist_) {
      return std::nullopt;
    }
    return hdr_value_at_percentile(hist_, pct);
  }

  void reset() noexcept {
    if (hist_) {
      hdr_reset(hist_);
    }
  }

 private:
  hdr_histogram* hist_{nullptr};
#else
  explicit HdrHistogramC(std::int64_t = 0, int = 3) {}
  [[nodiscard]] bool valid() const noexcept { return false; }
  void record(std::int64_t) noexcept {}
  [[nodiscard]] std::int64_t min() const noexcept { return 0; }
  [[nodiscard]] std::int64_t max() const noexcept { return 0; }
  [[nodiscard]] double mean() const noexcept { return 0.0; }
  [[nodiscard]] std::optional<std::int64_t> value_at_percentile(double) const noexcept {
    return std::nullopt;
  }
  void reset() noexcept {}
#endif
};

// Bridge: fill portable histogram from raw samples (for comparison tests).
inline void record_into_portable(HdrLatencyHistogram<>& portable, std::uint64_t v) {
  portable.record(v);
}

}  // namespace ll
