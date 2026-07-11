# Layer 5 — Compiler Exploitation & Language Modernity

Force the compiler to emit the machine code you mean — then **keep that code small**.

## Checklist

| Topic | Status |
|-------|--------|
| C++26 as project default | SHIPPED |
| constexpr / consteval | DOC + encourage in modules |
| CRTP / static polymorphism | `ll::StaticInterface` |
| `[[likely]]` / `[[unlikely]]` | `LL_LIKELY` / `LL_UNLIKELY` |
| LTO / PGO / ISA flags | DOC (portable defaults in CMake) |
| i-cache discipline | DOC (critical) |

## Compile-time work

```cpp
constexpr double tick_size = 0.01;
consteval int scale() { return 100; }
```

Push parsing of static config, hash tables of constants, and protocol tags to **compile time** when possible.

## Polymorphism without vtables

```cpp
struct Feed : ll::StaticInterface<Feed> {
  void on_tick() { /* ... */ }
};
// call feed.self().on_tick() — resolved statically, no vptr
```

Prefer `std::variant` + `std::visit` for closed sets of message types.

## Branch hints

```cpp
if (LL_UNLIKELY(error)) {
  handle_cold_path();
}
```

Cold error paths should not pollute the i-cache working set of the hot loop.

## Recommended flags (profile-dependent)

| Goal | Flags |
|------|--------|
| Portable release | `-O3 -flto` (optional) |
| Local max performance | `-O3 -flto -march=native` (**not** for portable artifacts) |
| PGO | build + train + rebuild with profile data |

This repo avoids hard-coding `-march=native` in shared CMake so CI and laptops stay portable; document native builds for production images.

## i-cache

**Instruction cache misses** from huge binaries often hurt more than one extra data load.

Practices:

- Split diagnostics / logging from hot TUs  
- Avoid heavy templates on the absolute critical path  
- Prefer few, predictable branches  
- Profile with `perf` (`icache`, `iTLB`)

## Module

```cpp
#include "ll/branch.hpp"
```
