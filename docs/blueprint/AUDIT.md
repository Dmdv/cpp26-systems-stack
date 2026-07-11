# Low-Latency Checklist Audit

Status legend:

| Status | Meaning |
|--------|---------|
| **SHIPPED** | Code + test and/or runnable example in this repo |
| **OPTIONAL** | Built when dependency is installed (auto-detect) |
| **DOC** | Documented with operator guidance; limited runtime API |
| **PARTIAL** | Touched via demo or third-party lib only |
| **GAP** | Not implemented here (reference / future module) |

Industry library map: [`07-industry-libraries.md`](07-industry-libraries.md) ·
Tutorial: [`../tutorials/industry-stack.md`](../tutorials/industry-stack.md).

---

## 1. Hardware & OS tuning

| Item | Status | Evidence |
|------|--------|----------|
| CPU isolation (`isolcpus`, `nohz_full`) | **DOC** | `ll/affinity.hpp` notes; `01-hardware-os.md` |
| Thread pinning `pthread_setaffinity_np` | **SHIPPED** (Linux) / **DOC** (macOS QoS) | `ll/affinity.hpp` |
| Topology discovery (**hwloc**) | **OPTIONAL** | `[industry][hwloc]` when lib present |
| NUMA (`numactl`, **libnuma**) | **DOC** / Linux **OPTIONAL** | soft CMake find + docs |
| C-states / P-states / governor | **DOC** | `01-hardware-os.md` |
| Hugepages / TLB | **DOC** | `01-hardware-os.md` |

---

## 2. Kernel bypass & network ingress

| Item | Status | Evidence |
|------|--------|----------|
| OpenOnload / EF_VI / **DPDK** | **GAP** | Documented envelope; not a driver product |
| **liburing** (`io_uring`) | Linux **OPTIONAL** | soft find when headers present |
| User-space NIC polling demo | **GAP** | Roadmap in `02-network-ingress.md` |
| Zero-copy kernel socket path | **PARTIAL** | Asio buffer-oriented APIs |
| **SBE** / packed POD | **SHIPPED** (style) + **DOC** (Real Logic tool) | `ll/sbe_style.hpp` |
| **FlatBuffers** | **OPTIONAL** | `schemas/tick.fbs`, industry tests |
| **struct_pack** | **DOC** | tutorial pointer |
| JSON / protobuf awareness | **SHIPPED** | simdjson, nlohmann-json, protobuf, gRPC |

---

## 3. Memory architecture & cache locality

| Item | Status | Evidence |
|------|--------|----------|
| No malloc on critical path (pattern) | **SHIPPED** | `ll/Arena`, `ll/ObjectPool`, **`ll/PmrMonotonicArena`** |
| **std::pmr** monotonic / unsync pool | **SHIPPED** | `ll/pmr_arena.hpp` |
| **mimalloc** / jemalloc (off path) | **OPTIONAL** / **DOC** | industry tests when installed |
| False sharing prevention | **SHIPPED** | `alignas(kCacheLine)` on SPSC head/tail |
| `hardware_destructive_interference_size` | **SHIPPED** | `ll/cache_line.hpp` |
| AoS vs SoA guidance | **DOC** | `03-memory.md` |

---

## 4. Concurrency & lock-free

| Item | Status | Evidence |
|------|--------|----------|
| SPSC lock-free ring (`ll`) | **SHIPPED** | `ll/spsc_queue.hpp` + stress test |
| **Boost.Lockfree** `spsc_queue` | **SHIPPED** | `[industry][boost_lockfree]` |
| **moodycamel::ReaderWriterQueue** | **OPTIONAL** | FetchContent + stress test |
| **folly::ProducerConsumerQueue** | **OPTIONAL** | Folly profile |
| acquire / release / relaxed explanation | **SHIPPED** | `04-concurrency.md` + examples |
| Mutex-free critical path pattern | **SHIPPED** | SPSC + single-thread actor guidance |
| Actor / single-thread hot path | **DOC** + related HFT repo | `04-concurrency.md` |
| MPSC / MPMC | **PARTIAL** | moodycamel ConcurrentQueue documented; not default |

---

## 5. Compiler exploitation & language modernity

| Item | Status | Evidence |
|------|--------|----------|
| C++26 project default | **SHIPPED** | `CMAKE_CXX_STANDARD 26` |
| constexpr / consteval guidance | **DOC** | `05-compiler.md` |
| CRTP / static polymorphism | **SHIPPED** | `ll/StaticInterface` |
| `[[likely]]` / `[[unlikely]]` | **SHIPPED** | `ll/branch.hpp` macros |
| LTO / PGO / march guidance | **DOC** | `05-compiler.md` |
| Keep critical path in L1 i-cache | **DOC** | blueprint intro + industry tutorial |

---

## 6. Benchmarking & telemetry

| Item | Status | Evidence |
|------|--------|----------|
| `perf` / PMC guidance | **DOC** | `06-telemetry.md` |
| RDTSC / `cntvct_el0` | **SHIPPED** | `ll/read_tsc()` |
| steady_clock ns | **SHIPPED** | `ll/steady_ns()` |
| Non-allocating sample buffer | **SHIPPED** | `ll/LatencyBuffer` |
| **HDR-style histogram** (p99 / p99.9 / max) | **SHIPPED** | `ll/HdrLatencyHistogram` |
| **HdrHistogram_c** | **DOC** | production upgrade path |
| **Google Benchmark** | **OPTIONAL** | `bench_queues` |
| Do not trust mean alone | **DOC** | tutorial + `example_hdr` |

---

## Focus summary

| Rank | Layer | Why |
|------|--------|-----|
| 1 | **§4 Concurrency** + **§3 Memory** | Portable primitives + industry queues (Boost / moodycamel) + pmr |
| 2 | **Library mesh** + **industry optional deps** | Asio/Beast/… plus hwloc, FlatBuffers, mimalloc, benchmark |
| 3 | **§6 Telemetry** | HDR percentiles + Google Benchmark when present |
| 4 | **§1 / §5** | hwloc + operator / compiler guidance |
| 5 | **§2 Kernel bypass** | Documented boundary; SBE-style + FlatBuffers on portable path |

### Next recommended investment

1. Real Logic **SBE codegen** for production schemas  
2. Linux **numa + uring** demos in CI on Linux runners  
3. Optional **HdrHistogram_c** wrapper next to `ll::HdrLatencyHistogram`  
4. DPDK / Onload only with dedicated hardware lab

This audit is the checklist for PRs: new work should move a row from **DOC/GAP → SHIPPED/OPTIONAL** with tests.
