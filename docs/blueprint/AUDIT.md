# Low-Latency Checklist Audit

Status legend:

| Status | Meaning |
|--------|---------|
| **SHIPPED** | Code + test and/or runnable example in this repo |
| **DOC** | Documented with operator guidance; limited runtime API |
| **PARTIAL** | Touched via third-party lib or demo only |
| **GAP** | Not implemented here (reference / future module) |

---

## 1. Hardware & OS tuning

| Item | Status | Evidence |
|------|--------|----------|
| CPU isolation (`isolcpus`, `nohz_full`) | **DOC** | `ll/affinity.hpp` notes; `01-hardware-os.md` |
| Thread pinning `pthread_setaffinity_np` | **SHIPPED** (Linux) / **DOC** (macOS QoS) | `ll/affinity.hpp` |
| NUMA (`numactl`, libnuma) | **DOC** | `01-hardware-os.md` |
| C-states / P-states / governor | **DOC** | `01-hardware-os.md` |
| Hugepages / TLB | **DOC** | `01-hardware-os.md` |

---

## 2. Kernel bypass & network ingress

| Item | Status | Evidence |
|------|--------|----------|
| OpenOnload / EF_VI / DPDK | **GAP** | Documented as envelope; not a driver product |
| User-space NIC polling demo | **GAP** | Roadmap in `02-network-ingress.md` |
| Zero-copy from kernel socket path | **PARTIAL** | Asio buffer-oriented APIs; not NIC rings |
| SBE / FlatBuffers / packed POD | **PARTIAL** | POD + binary mindset in guides; SBE not vendored |
| JSON / protobuf awareness | **SHIPPED** | simdjson, nlohmann-json, protobuf, gRPC |

---

## 3. Memory architecture & cache locality

| Item | Status | Evidence |
|------|--------|----------|
| No malloc on critical path (pattern) | **SHIPPED** | `ll/Arena`, `ll/ObjectPool` |
| False sharing prevention | **SHIPPED** | `alignas(kCacheLine)` on SPSC head/tail |
| `hardware_destructive_interference_size` | **SHIPPED** | `ll/cache_line.hpp` |
| AoS vs SoA guidance | **DOC** | `03-memory.md` |
| Integration binary still demos libs | **PARTIAL** | `src/main.cpp` is not a hot-path product |

---

## 4. Concurrency & lock-free

| Item | Status | Evidence |
|------|--------|----------|
| SPSC lock-free ring | **SHIPPED** | `ll/spsc_queue.hpp` + stress test |
| acquire / release / relaxed explanation | **SHIPPED** | `04-concurrency.md` + `examples/memory_order` |
| Mutex-free critical path pattern | **SHIPPED** | SPSC + single-thread actor guidance |
| Actor / single-thread hot path | **DOC** + related HFT repo | `04-concurrency.md` |
| MPSC / MPMC | **GAP** | Not in `ll` yet |
| Taskflow / TBB / stdexec | **SHIPPED** | library guides + tests |

---

## 5. Compiler exploitation & language modernity

| Item | Status | Evidence |
|------|--------|----------|
| C++26 project default | **SHIPPED** | `CMAKE_CXX_STANDARD 26` |
| constexpr / consteval guidance | **DOC** | `05-compiler.md` |
| CRTP / static polymorphism | **SHIPPED** | `ll/StaticInterface` |
| `[[likely]]` / `[[unlikely]]` | **SHIPPED** | `ll/branch.hpp` macros |
| LTO / PGO / march guidance | **DOC** | `05-compiler.md` (portable flags preferred) |
| Keep critical path in L1 i-cache | **DOC** | emphasized in blueprint intro |

---

## 6. Benchmarking & telemetry

| Item | Status | Evidence |
|------|--------|----------|
| `perf` / PMC guidance | **DOC** | `06-telemetry.md` |
| RDTSC / `cntvct_el0` | **SHIPPED** | `ll/read_tsc()` |
| steady_clock ns | **SHIPPED** | `ll/steady_ns()` |
| Non-allocating sample buffer | **SHIPPED** | `ll/LatencyBuffer` |
| p99 / p99.9 / max emphasis | **DOC** + sample buffer | full HDR histogram **GAP** |
| Do not trust mean alone | **DOC** | `06-telemetry.md` |

---

## Focus summary

| Rank | Layer | Why |
|------|--------|-----|
| 1 | **§4 Concurrency** + **§3 Memory** | Portable primitives every low-latency app needs |
| 2 | **Library mesh** (Asio, Beast, Taskflow, TBB, stdexec, simdjson, gRPC) | Modern C++26 *ecosystem* integration |
| 3 | **§6 Telemetry primitives** | Cycle/ns stamps + fixed sample buffers |
| 4 | **§1 / §5** | Operator + compiler guidance |
| 5 | **§2 Kernel bypass** | Documented boundary; specialized hardware repos own drivers |

This audit is the checklist for PRs: new work should move a row from **DOC/GAP → SHIPPED** with tests.
