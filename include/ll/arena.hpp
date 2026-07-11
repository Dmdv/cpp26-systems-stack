#pragma once

#include "ll/cache_line.hpp"

#include <cstddef>
#include <cstdint>
#include <new>
#include <span>
#include <stdexcept>
#include <vector>

namespace ll {

// Bump (arena) allocator for critical-path object construction.
// Reset between messages / windows — never free individual objects on the hot path.
//
// See docs/blueprint/03-memory.md
class Arena {
 public:
  explicit Arena(std::size_t bytes)
      : storage_(bytes), offset_(0) {}

  Arena(const Arena&) = delete;
  Arena& operator=(const Arena&) = delete;

  // Allocate `n` bytes aligned to `align` (must be power of two).
  [[nodiscard]] void* allocate(std::size_t n, std::size_t align = alignof(std::max_align_t)) {
    const auto base = reinterpret_cast<std::uintptr_t>(storage_.data() + offset_);
    const auto aligned = (base + (align - 1)) & ~(static_cast<std::uintptr_t>(align) - 1);
    const auto delta = static_cast<std::size_t>(aligned - base);
    if (offset_ + delta + n > storage_.size()) {
      throw std::bad_alloc{};
    }
    offset_ += delta + n;
    return reinterpret_cast<void*>(aligned);
  }

  template <class T, class... Args>
  [[nodiscard]] T* create(Args&&... args) {
    void* mem = allocate(sizeof(T), alignof(T));
    return new (mem) T(std::forward<Args>(args)...);
  }

  void reset() noexcept { offset_ = 0; }

  [[nodiscard]] std::size_t used() const noexcept { return offset_; }
  [[nodiscard]] std::size_t capacity() const noexcept { return storage_.size(); }
  [[nodiscard]] std::span<std::byte> remaining() noexcept {
    return std::span<std::byte>(storage_.data() + offset_, storage_.size() - offset_);
  }

 private:
  alignas(kCacheLine) std::vector<std::byte> storage_;
  std::size_t offset_;
};

// Fixed object pool — pre-construct N objects, checkout/return without the OS allocator.
template <class T, std::size_t N>
class ObjectPool {
 public:
  ObjectPool() {
    for (std::size_t i = 0; i < N; ++i) {
      free_[i] = &slots_[i];
    }
    top_ = N;
  }

  [[nodiscard]] T* acquire() noexcept {
    if (top_ == 0) {
      return nullptr;
    }
    return free_[--top_];
  }

  void release(T* p) noexcept {
    if (p == nullptr || top_ >= N) {
      return;
    }
    free_[top_++] = p;
  }

  [[nodiscard]] std::size_t available() const noexcept { return top_; }

 private:
  T slots_[N]{};
  T* free_[N]{};
  std::size_t top_{0};
};

}  // namespace ll
