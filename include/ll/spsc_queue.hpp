#pragma once

#include "ll/cache_line.hpp"

#include <atomic>
#include <cstddef>
#include <optional>
#include <type_traits>
#include <utility>

namespace ll {

// Bounded lock-free SPSC ring (power-of-two capacity).
// Single producer / single consumer only — the backbone of low-latency pipelines.
//
// Memory orders:
//   producer: write slot, then release-store tail
//   consumer: acquire-load tail, read slot, then release-store head
// See docs/blueprint/04-concurrency.md
template <class T, std::size_t Capacity>
  requires(Capacity >= 2 && (Capacity & (Capacity - 1)) == 0)
class SpscQueue {
 public:
  static constexpr std::size_t capacity = Capacity;
  static constexpr std::size_t mask = Capacity - 1;

  SpscQueue() = default;
  SpscQueue(const SpscQueue&) = delete;
  SpscQueue& operator=(const SpscQueue&) = delete;

  // Producer only.
  [[nodiscard]] bool try_push(const T& item) noexcept(
      std::is_nothrow_copy_assignable_v<T>) {
    const auto t = tail_.load(std::memory_order_relaxed);
    const auto next = (t + 1) & mask;
    if (next == head_.load(std::memory_order_acquire)) {
      return false;  // full
    }
    slots_[t] = item;
    tail_.store(next, std::memory_order_release);
    return true;
  }

  [[nodiscard]] bool try_push(T&& item) noexcept(
      std::is_nothrow_move_assignable_v<T>) {
    const auto t = tail_.load(std::memory_order_relaxed);
    const auto next = (t + 1) & mask;
    if (next == head_.load(std::memory_order_acquire)) {
      return false;
    }
    slots_[t] = std::move(item);
    tail_.store(next, std::memory_order_release);
    return true;
  }

  // Consumer only.
  [[nodiscard]] std::optional<T> try_pop() noexcept(
      std::is_nothrow_move_constructible_v<T>) {
    const auto h = head_.load(std::memory_order_relaxed);
    if (h == tail_.load(std::memory_order_acquire)) {
      return std::nullopt;  // empty
    }
    T out = std::move(slots_[h]);
    head_.store((h + 1) & mask, std::memory_order_release);
    return out;
  }

  [[nodiscard]] bool empty() const noexcept {
    return head_.load(std::memory_order_acquire) ==
           tail_.load(std::memory_order_acquire);
  }

 private:
  alignas(kCacheLine) std::atomic<std::size_t> head_{0};
  alignas(kCacheLine) std::atomic<std::size_t> tail_{0};
  alignas(kCacheLine) T slots_[Capacity]{};
};

}  // namespace ll
