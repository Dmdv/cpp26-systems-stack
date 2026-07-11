#pragma once

// Branch-weight hints for hot paths (C++20).
// Keep critical-path code small enough for L1 i-cache — see docs/blueprint/05-compiler.md

#if defined(__has_cpp_attribute)
#if __has_cpp_attribute(likely)
#define LL_LIKELY(x) (x) [[likely]]
#define LL_UNLIKELY(x) (x) [[unlikely]]
#endif
#endif

#ifndef LL_LIKELY
#if defined(__GNUC__) || defined(__clang__)
#define LL_LIKELY(x) (__builtin_expect(!!(x), 1))
#define LL_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
#define LL_LIKELY(x) (x)
#define LL_UNLIKELY(x) (x)
#endif
#endif

namespace ll {

// CRTP base for static polymorphism without vtables on the hot path.
template <class Derived>
struct StaticInterface {
  constexpr Derived& self() noexcept { return static_cast<Derived&>(*this); }
  constexpr const Derived& self() const noexcept {
    return static_cast<const Derived&>(*this);
  }
};

}  // namespace ll
