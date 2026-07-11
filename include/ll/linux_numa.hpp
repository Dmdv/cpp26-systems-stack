#pragma once

// NUMA helpers (Linux + libnuma). On other platforms these are no-ops / reports.
// See docs/tutorials/linux-numa-uring.md

#include <cstddef>
#include <cstdint>
#include <string>

#ifndef LL_HAS_LIBNUMA
#define LL_HAS_LIBNUMA 0
#endif

#if LL_HAS_LIBNUMA
#include <numa.h>
#include <numaif.h>
#endif

namespace ll::numa {

[[nodiscard]] inline bool available() noexcept {
#if LL_HAS_LIBNUMA
  return numa_available() != -1;
#else
  return false;
#endif
}

[[nodiscard]] inline int max_node() noexcept {
#if LL_HAS_LIBNUMA
  if (!available()) {
    return -1;
  }
  return numa_max_node();
#else
  return -1;
#endif
}

// Preferred node for the calling thread (may be -1 if unknown).
[[nodiscard]] inline int preferred_node() noexcept {
#if LL_HAS_LIBNUMA
  if (!available()) {
    return -1;
  }
  return numa_preferred();
#else
  return -1;
#endif
}

// Bind current thread CPU affinity + memory policy to a single NUMA node.
// Returns false if NUMA is unavailable or bind failed.
[[nodiscard]] inline bool bind_thread_to_node(int node) noexcept {
#if LL_HAS_LIBNUMA
  if (!available() || node < 0 || node > numa_max_node()) {
    return false;
  }
  bitmask* cpus = numa_allocate_cpumask();
  if (!cpus) {
    return false;
  }
  const int rc_cpus = numa_node_to_cpus(node, cpus);
  if (rc_cpus != 0) {
    numa_free_cpumask(cpus);
    return false;
  }
  const int rc_run = numa_run_on_node_mask(cpus);
  numa_free_cpumask(cpus);
  if (rc_run != 0) {
    return false;
  }
  numa_set_preferred(node);
  return true;
#else
  (void)node;
  return false;
#endif
}

// Allocate `bytes` on `node` (numa_alloc_onnode). Free with free_onnode.
[[nodiscard]] inline void* alloc_onnode(std::size_t bytes, int node) noexcept {
#if LL_HAS_LIBNUMA
  if (!available() || bytes == 0) {
    return nullptr;
  }
  return numa_alloc_onnode(bytes, node);
#else
  (void)bytes;
  (void)node;
  return nullptr;
#endif
}

inline void free_onnode(void* p, std::size_t bytes) noexcept {
#if LL_HAS_LIBNUMA
  if (p) {
    numa_free(p, bytes);
  }
#else
  (void)p;
  (void)bytes;
#endif
}

[[nodiscard]] inline const char* backend() noexcept {
#if LL_HAS_LIBNUMA
  return available() ? "libnuma" : "libnuma-unavailable";
#else
  return "stub";
#endif
}

[[nodiscard]] inline std::string status_line() {
  std::string s = "numa_backend=";
  s += backend();
  s += " available=";
  s += available() ? "yes" : "no";
  s += " max_node=";
  s += std::to_string(max_node());
  s += " preferred=";
  s += std::to_string(preferred_node());
  return s;
}

}  // namespace ll::numa
