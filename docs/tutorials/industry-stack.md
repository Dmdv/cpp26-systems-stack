# Tutorial: Industry libraries on the low-latency stack

This walkthrough maps **battle-tested C++ libraries** onto each layer of the
blueprint, shows what this repo **builds by default**, and how to run the
demos. Pair with [`docs/blueprint/AUDIT.md`](../blueprint/AUDIT.md).

---

## Mental model

```text
┌─────────────────────────────────────────────────────────────┐
│  §1 Hardware / OS     hwloc · libnuma · isolcpus (ops)      │
├─────────────────────────────────────────────────────────────┤
│  §2 Network ingress   DPDK / Onload (GAP) · liburing (Linux)│
│                       Asio/Beast · SBE-style · FlatBuffers  │
├─────────────────────────────────────────────────────────────┤
│  §3 Memory            ll::Arena · std::pmr · mimalloc(off)  │
├─────────────────────────────────────────────────────────────┤
│  §4 Concurrency       ll::SpscQueue · moodycamel · Boost LF │
│                       folly::PCQ (optional Folly profile)   │
├─────────────────────────────────────────────────────────────┤
│  §5 Compiler          C++26 · CRTP · LTO/PGO guidance       │
├─────────────────────────────────────────────────────────────┤
│  §6 Telemetry         ll::HdrLatencyHistogram · Google Bench│
│                       ll::read_tsc / steady_ns              │
└─────────────────────────────────────────────────────────────┘
```

**Design rule:** use industry libraries **off or at the edges** of the critical
path when they help correctness/speed; keep the **hottest loop** small enough
for L1 i-cache (your own SPSC + POD + arena often wins on footprint).

---

## 1. Hardware & topology — `hwloc`

**Why:** programmatic view of cores, caches, NUMA nodes — better than hardcoding
core IDs when machines differ.

```bash
brew install hwloc          # macOS
# Linux: libhwloc-dev / hwloc-devel
```

CMake auto-enables when headers + `libhwloc` are found (`STACK_WITH_HWLOC=ON`).

```cpp
#include <hwloc.h>
hwloc_topology_t topo;
hwloc_topology_init(&topo);
hwloc_topology_load(topo);
int cores = hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_CORE);
// pin with hwloc_set_cpubind / combine with ll::pin_current_thread_to_cpu
hwloc_topology_destroy(topo);
```

**libnuma / numactl (Linux):** bind memory to the socket that runs the thread.
Not linked on macOS; see `01-hardware-os.md`. Soft-detect via `STACK_WITH_LIBNUMA`.

---

## 2. Network ingress

| Library | Role | In this repo |
|---------|------|----------------|
| **DPDK** | User-space NIC polling | **GAP** — ops/docs only |
| **OpenOnload / libef_vi** | Solarflare HFT path | **GAP** — vendor SDK |
| **liburing** | Modern async I/O without classic syscall storms | Linux soft-detect |
| **Asio / Beast** | Portable sockets / HTTP / WS | **SHIPPED** |
| **SBE (Real Logic)** | Wire-speed binary codecs | **SBE-style POD** demo + generate-with-SBE guide |
| **FlatBuffers** | Zero-copy tables | **SHIPPED** when `flatbuffers` + `flatc` installed |
| **struct_pack** | Compile-time C++20 pack | Documented alternative (not vendored) |

### SBE-style packed tick

```bash
./build/example_sbe_style
```

Uses `ll::sbe::TickMsg` (`#pragma pack(1)`, 16 bytes). For production, generate
codecs from XML with [simple-binary-encoding](https://github.com/real-logic/simple-binary-encoding).

### FlatBuffers

```bash
brew install flatbuffers
# schema: schemas/tick.fbs → generated at build
ctest --test-dir build -R flatbuffers
```

Access fields without parsing into a second object graph — ideal for fan-out
from a shared buffer (still not a replacement for SBE on the absolute hottest
exchange-facing path).

---

## 3. Memory — `std::pmr` + mimalloc

### Critical path: arenas

```cpp
#include "ll/pmr_arena.hpp"
ll::PmrMonotonicArena arena(1 << 20);
auto* x = arena.create<Order>(...);
// … process window …
arena.release();  // free everything at once
```

Also: `ll::Arena`, `ll::ObjectPool`, `ll::PmrUnsyncPool` (single-thread pool).

### Off path: mimalloc / jemalloc

```bash
brew install mimalloc   # or jemalloc
```

Use for startup, admin threads, logging — **not** inside the tick loop.
CMake sets `LL_HAS_MIMALLOC=1` when found.

---

## 4. Lock-free queues

| Queue | Best for | How to run |
|-------|----------|------------|
| `ll::SpscQueue` | Teaching fences + tiny i-cache | `test_ll_modules` |
| `boost::lockfree::spsc_queue` | Boost already in tree | `ctest -R boost_lockfree` |
| `moodycamel::ReaderWriterQueue` | Industry SPSC benchmark | FetchContent; `ctest -R moodycamel` |
| `folly::ProducerConsumerQueue` | Folly profile | `make folly` + Folly tests |
| `moodycamel::ConcurrentQueue` | MPMC (not SPSC) | Documented; not default |

```bash
./build/example_industry_queues
```

**Memory order recap (SPSC):** producer stores payload then `release` on tail;
consumer `acquire`s tail, reads payload, `release`s head. Never share a queue
across multiple producers without an MPMC structure.

---

## 5. Telemetry — HDR histograms + Google Benchmark

### Tail latency (always on)

```bash
./build/example_hdr
# prints p50 / p99 / p99.9 / max — mean alone is not enough
```

`ll::HdrLatencyHistogram` is allocation-free after construction (log2 buckets).
For highest-fidelity production metrics, pair with
[HdrHistogram_c](https://github.com/HdrHistogram/HdrHistogram_c).

### Microbenchmarks

```bash
brew install google-benchmark
cmake --build build --target bench_queues
./build/bench_queues --benchmark_filter=spsc
```

Google Benchmark prevents the compiler from DCE’ing empty loops and reports
time/iteration for `ll` vs Boost vs moodycamel.

---

## 6. Install matrix (macOS Homebrew)

```bash
# Base systems stack (already required)
brew install cmake fmt spdlog tbb asio boost taskflow \
  nlohmann-json simdjson eigen protobuf grpc \
  range-v3 abseil catch2 googletest

# Industry low-latency add-ons
brew install hwloc flatbuffers google-benchmark mimalloc
# optional: jemalloc folly
```

```bash
make distclean
make                 # configure + build + ctest
make examples        # includes new pmr/hdr/sbe/industry demos
make bench           # if benchmark found
```

CMake options (all default ON; disabled automatically when missing):

| Option | Effect |
|--------|--------|
| `STACK_WITH_HWLOC` | topology smoke tests |
| `STACK_WITH_FLATBUFFERS` | schema codegen + tests |
| `STACK_WITH_BENCHMARK` | `bench_queues` target |
| `STACK_WITH_MIMALLOC` | link mimalloc in industry tests |
| `STACK_WITH_MOODYCAMEL` | FetchContent ReaderWriterQueue |
| `STACK_WITH_LIBNUMA` / `STACK_WITH_LIBURING` | Linux only |

---

## 7. Roadmap deep-dives (implemented)

| Topic | Tutorial |
|-------|----------|
| Real Logic SBE codegen | [sbe-codegen.md](sbe-codegen.md) |
| Linux NUMA + io_uring | [linux-numa-uring.md](linux-numa-uring.md) |
| HdrHistogram_c | [hdrhistogram-c.md](hdrhistogram-c.md) |
| DPDK / Onload lab | [kernel-bypass-lab.md](kernel-bypass-lab.md) |

---

## Related files

| Path | Purpose |
|------|---------|
| `include/ll/pmr_arena.hpp` | std::pmr wrappers |
| `include/ll/hdr_histogram.hpp` / `hdr_c.hpp` | portable + HdrHistogram_c |
| `include/ll/sbe_style.hpp` / `sbe_codec.hpp` | POD + generated SBE |
| `include/ll/linux_numa.hpp` / `linux_uring.hpp` | Linux backends |
| `include/ll/kernel_bypass.hpp` | poll-mode RX contract |
| `schemas/tick.fbs` · `sbe-market-data-schema.xml` | wire schemas |
| `generated/sbe/` | committed SBE C++ codecs |
| `tests/test_industry_stack.cpp` · `test_roadmap_stack.cpp` | suites |
| `benchmarks/bench_queues.cpp` | Google Benchmark |
| `docs/blueprint/07-industry-libraries.md` | layer ↔ library map |
