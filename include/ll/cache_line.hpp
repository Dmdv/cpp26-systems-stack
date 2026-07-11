#pragma once

#include <cstddef>
#include <new>

namespace ll {

// Portable destructive interference size (cache line) for false-sharing guards.
// Prefer the standard constant when the library provides it.
#if defined(__cpp_lib_hardware_interference_size)
inline constexpr std::size_t kCacheLine =
    std::hardware_destructive_interference_size;
#else
inline constexpr std::size_t kCacheLine = 64;
#endif

// Pad a type so consecutive instances do not share a cache line.
template <class T>
struct alignas(kCacheLine) CacheLinePadded {
  T value{};
};

}  // namespace ll
