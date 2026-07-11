# Layer 3 — Memory Architecture & Cache Locality

**Rule:** the fastest memory access is the one you never make.  
**Rule 2:** never call the OS allocator on the critical path.

## Checklist

| Topic | Module |
|-------|--------|
| Arena / bump allocation | `ll::Arena` |
| Object pools | `ll::ObjectPool<T,N>` |
| Cache-line padding | `ll::kCacheLine`, `CacheLinePadded` |
| False sharing on queues | SPSC head/tail `alignas` |
| AoS vs SoA | Guidance below |

## Zero allocation on the critical path

```cpp
#include "ll/arena.hpp"

ll::Arena arena(1 << 20);  // pre-size at startup
// per message / window:
arena.reset();
auto* tmp = arena.create<MyState>(/*...*/);
```

```cpp
ll::ObjectPool<Order, 4096> orders;  // pre-constructed
Order* o = orders.acquire();
// ...
orders.release(o);
```

## False sharing

Two atomics on the same 64-byte line → bouncing cache lines between cores.

```cpp
alignas(ll::kCacheLine) std::atomic<std::size_t> head;
alignas(ll::kCacheLine) std::atomic<std::size_t> tail;
```

`ll::SpscQueue` does this by construction.

## Data-oriented layout

| Layout | Good when |
|--------|-----------|
| **AoS** (`struct {px,qty}[]`) | Processing whole records together |
| **SoA** (`px[]`, `qty[]`) | SIMD math over one field (VWAP qty*px streams) |

```text
AoS cache line: [px0 qty0 px1 qty1 ...]  — may waste bandwidth if only px needed
SoA cache line: [px0 px1 px2 px3 ...]    — denser for SIMD
```

## Example & tests

- Example: `examples/arena/main.cpp`  
- Tests: `[ll][arena]`, `[ll][pool]`, `[ll][cache]`
