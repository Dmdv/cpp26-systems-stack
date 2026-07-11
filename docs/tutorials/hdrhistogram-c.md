# Tutorial: HdrHistogram_c + portable HDR

Tail latency is the product metric. This repo ships **two** histogram layers:

| Layer | Type | When to use |
|-------|------|-------------|
| `ll::HdrLatencyHistogram` | Portable log2 buckets, header-only | Always-on, no deps, demos |
| `ll::HdrHistogramC` | Wrapper over **HdrHistogram_c** | Production fidelity (optional) |

## Enable HdrHistogram_c

CMake option `STACK_WITH_HDRHISTOGRAM=ON` (default) FetchContents
[HdrHistogram_c](https://github.com/HdrHistogram/HdrHistogram_c) **0.11.10**
and links `hdr_histogram_static`.

```text
LL_HAS_HDRHISTOGRAM_C=1  → full API
LL_HAS_HDRHISTOGRAM_C=0  → wrapper methods no-op / invalid()
```

Disable if you need a deps-free configure:

```bash
cmake -S . -B build -DSTACK_WITH_HDRHISTOGRAM=OFF
```

## Usage

```cpp
#include "ll/hdr_c.hpp"
#include "ll/tsc_clock.hpp"

ll::HdrHistogramC h;  // range up to ~1h in ns by default
auto t0 = ll::steady_ns();
// … work …
h.record(static_cast<std::int64_t>(ll::steady_ns() - t0));

auto p99  = h.value_at_percentile(99.0);
auto p999 = h.value_at_percentile(99.9);
auto mx   = h.max();
```

Compare with portable:

```cpp
ll::HdrLatencyHistogram<> portable;
portable.record(dt);
// portable is coarser (power-of-two buckets) but zero external deps
```

## Run

```bash
./build/example_hdr_c
ctest --test-dir build -R 'roadmap.*hdr'
```

## Production notes

1. Prefer **HdrHistogram_c** (or Java/Go HDR ports) for long-running services.  
2. Record in **nanoseconds or cycles**, not mixed units.  
3. Export p50/p90/p99/p99.9/max every interval — never ship on mean alone.  
4. Keep recording **allocation-free** (both wrappers avoid heap after init).  
5. Pair with Google Benchmark (`make bench`) for micro-level queue costs.

## Related

- `docs/blueprint/06-telemetry.md`  
- `include/ll/hdr_histogram.hpp` · `include/ll/hdr_c.hpp`
