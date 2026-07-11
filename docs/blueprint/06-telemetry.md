# Layer 6 — Benchmarking & Telemetry

If you only track **mean** latency, you will ship a system that fails in production.  
HFT and ULL systems live and die on **tails**.

## Checklist

| Topic | Module / tool |
|-------|----------------|
| Cycle counters | `ll::read_tsc()` (RDTSC / `cntvct_el0`) |
| Wall-ish mono ns | `ll::steady_ns()` |
| Fixed sample buffer | `ll::LatencyBuffer<N>` |
| **HDR-style histogram** | `ll::HdrLatencyHistogram` — p50/p99/p99.9/max |
| **Google Benchmark** | `bench_queues` (optional package) |
| **HdrHistogram_c** | Production-grade upgrade path (**DOC**) |
| `perf` PMCs | Operator guide below |

```bash
./build/example_hdr
make bench
```

## Timestamps

```cpp
#include "ll/tsc_clock.hpp"

const auto t0 = ll::read_tsc();
do_work();
const auto dt = ll::read_tsc() - t0;
```

| Platform | Counter |
|----------|---------|
| x86_64 | `__rdtsc()` |
| aarch64 | `cntvct_el0` |
| fallback | `steady_clock` ns |

**Caveat:** compare cycle counts only with a **pinned** thread on a stable frequency domain.

## Non-allocating capture

```cpp
ll::LatencyBuffer<1<<20> hist;
hist.try_push(delta_ns);  // never allocates
// after run: sort copy → percentiles
```

## Tail latency

| Metric | Why |
|--------|-----|
| p50 | Typical case |
| p99 | Common SLO language |
| p99.9 / max | Outliers that blow risk limits |

Mean is a **vanity metric** when the distribution is heavy-tailed.

## Hardware performance counters

```bash
perf stat -e cycles,instructions,cache-misses,branch-misses ./app
perf record -e cycles:u ./app && perf report
```

Watch:

- IPC (instructions per cycle)  
- cache-misses  
- branch-misses  
- context-switches / migrations (OS noise)

## Microbenchmark honesty

- Prevent DCE: use `volatile`/`DoNotOptimize` sinks  
- Warm up code and data pages  
- Pin threads before measuring  
- Prefer distribution reports over single-number “ns/op”

## Example & tests

- `examples/tsc/main.cpp`  
- `[ll][tsc]` in `tests/test_ll_modules.cpp`

## Roadmap

- HDR histogram (no alloc, compressed bins)  
- Optional integration with `perf` counters via `perf_event_open` on Linux  
