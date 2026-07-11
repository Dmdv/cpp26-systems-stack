#pragma once

// std::pmr wrappers for critical-path arenas / thread-local pools.
// Prefer these (or ll::Arena) over malloc/new on the hot path.
// See docs/blueprint/03-memory.md and docs/tutorials/industry-stack.md

#include "ll/cache_line.hpp"

#include <cstddef>
#include <memory_resource>
#include <new>
#include <span>
#include <vector>

namespace ll {

// Stack/heap-backed monotonic arena (std::pmr::monotonic_buffer_resource).
// Allocate freely during a window; release() once at the end.
class PmrMonotonicArena {
 public:
  explicit PmrMonotonicArena(std::size_t bytes)
      : buffer_(bytes),
        resource_(buffer_.data(), buffer_.size(), std::pmr::null_memory_resource()) {}

  PmrMonotonicArena(const PmrMonotonicArena&) = delete;
  PmrMonotonicArena& operator=(const PmrMonotonicArena&) = delete;

  [[nodiscard]] std::pmr::memory_resource* resource() noexcept { return &resource_; }

  template <class T, class... Args>
  [[nodiscard]] T* create(Args&&... args) {
    void* mem = resource_.allocate(sizeof(T), alignof(T));
    return new (mem) T(std::forward<Args>(args)...);
  }

  // Destroy is optional for POD; for non-trivial T call destroy before release.
  template <class T>
  void destroy(T* p) noexcept {
    if (p) {
      p->~T();
    }
  }

  void release() noexcept { resource_.release(); }

  [[nodiscard]] std::size_t capacity() const noexcept { return buffer_.size(); }

 private:
  alignas(kCacheLine) std::vector<std::byte> buffer_;
  std::pmr::monotonic_buffer_resource resource_;
};

// unsynchronized_pool_resource — intended for single-threaded critical paths.
// Not lock-free across threads; pair one instance per thread.
class PmrUnsyncPool {
 public:
  explicit PmrUnsyncPool(std::pmr::memory_resource* upstream = std::pmr::new_delete_resource())
      : pool_(std::pmr::pool_options{}, upstream) {}

  [[nodiscard]] std::pmr::memory_resource* resource() noexcept { return &pool_; }

  template <class T, class... Args>
  [[nodiscard]] T* create(Args&&... args) {
    void* mem = pool_.allocate(sizeof(T), alignof(T));
    return new (mem) T(std::forward<Args>(args)...);
  }

  template <class T>
  void destroy_and_deallocate(T* p) noexcept {
    if (!p) {
      return;
    }
    p->~T();
    pool_.deallocate(p, sizeof(T), alignof(T));
  }

 private:
  std::pmr::unsynchronized_pool_resource pool_;
};

}  // namespace ll
