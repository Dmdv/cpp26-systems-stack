#pragma once

// Optional jemalloc helpers for off-critical-path allocations.
// Prefer ll::Arena / std::pmr on the hot path.
// Enabled when LL_HAS_JEMALLOC=1.

#include <cstddef>
#include <cstdlib>
#include <string>

#ifndef LL_HAS_JEMALLOC
#define LL_HAS_JEMALLOC 0
#endif

#if LL_HAS_JEMALLOC
#include <jemalloc/jemalloc.h>
#endif

namespace ll::je {

[[nodiscard]] inline bool available() noexcept {
#if LL_HAS_JEMALLOC
  return true;
#else
  return false;
#endif
}

[[nodiscard]] inline void* malloc_bytes(std::size_t n) noexcept {
#if LL_HAS_JEMALLOC
  // Homebrew jemalloc may expose malloc via je_* macros in the header.
  return ::malloc(n);
#else
  return std::malloc(n);
#endif
}

inline void free_bytes(void* p) noexcept {
#if LL_HAS_JEMALLOC
  ::free(p);
#else
  std::free(p);
#endif
}

// Version string when jemalloc is linked; "stub" otherwise.
[[nodiscard]] inline std::string version_string() {
#if LL_HAS_JEMALLOC
  const char* v = nullptr;
  std::size_t sz = sizeof(v);
  if (::mallctl("version", &v, &sz, nullptr, 0) == 0 && v != nullptr) {
    return std::string(v);
  }
  return "jemalloc";
#else
  return "stub";
#endif
}

}  // namespace ll::je
