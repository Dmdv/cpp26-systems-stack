# Low-Latency Checklist Audit

Status legend:

| Status | Meaning |
|--------|---------|
| **SHIPPED** | Code + test and/or runnable example in this repo |
| **OPTIONAL** | Built when dependency is installed (auto-detect) |
| **DOC** | Documented with operator guidance; limited runtime API |
| **PARTIAL** | Touched via demo or third-party lib only |
| **GAP** | Not implemented here (reference / future module) |

Industry map: [`07-industry-libraries.md`](07-industry-libraries.md) ·  
Tutorials: [`../tutorials/`](../tutorials/).

---

## 1. Hardware & OS tuning

| Item | Status | Evidence |
|------|--------|----------|
| CPU isolation (`isolcpus`, `nohz_full`) | **DOC** | `01-hardware-os.md` |
| Thread pinning | **SHIPPED** / macOS QoS **DOC** | `ll/affinity.hpp` |
| **hwloc** topology | **OPTIONAL** | `[industry][hwloc]` |
| **libnuma** bind / alloc_onnode | **SHIPPED** API + Linux **OPTIONAL** link | `ll/linux_numa.hpp`, `[roadmap][numa]`, Linux CI |
| C-states / P-states / governor | **DOC** | `01-hardware-os.md` |
| Hugepages / TLB | **DOC** | `01-hardware-os.md`, kernel-bypass lab |

---

## 2. Kernel bypass & network ingress

| Item | Status | Evidence |
|------|--------|----------|
| **IPollModeRx** contract + stub | **SHIPPED** | `ll/kernel_bypass.hpp`, `[roadmap][bypass]` |
| **DPDK** implementation | **GAP** (lab runbook) | `docs/tutorials/kernel-bypass-lab.md` |
| **OpenOnload / EF_VI** implementation | **GAP** (lab runbook) | same |
| **liburing** | **SHIPPED** API + Linux **OPTIONAL** | `ll/linux_uring.hpp`, Linux CI |
| **Real Logic SBE codegen** | **SHIPPED** | `schemas/sbe-*.xml`, `generated/sbe/`, `ll/sbe_codec.hpp` |
| SBE-style POD teaching aid | **SHIPPED** | `ll/sbe_style.hpp` |
| **FlatBuffers** | **OPTIONAL** | `schemas/tick.fbs` |
| Asio / Beast / gRPC / simdjson | **SHIPPED** | library mesh |

---

## 3. Memory architecture & cache locality

| Item | Status | Evidence |
|------|--------|----------|
| Arena / object pool | **SHIPPED** | `ll/arena.hpp` |
| **std::pmr** | **SHIPPED** | `ll/pmr_arena.hpp` |
| **mimalloc** off-path | **OPTIONAL** | industry tests |
| False sharing / cache lines | **SHIPPED** | `ll/cache_line.hpp`, SPSC |

---

## 4. Concurrency & lock-free

| Item | Status | Evidence |
|------|--------|----------|
| `ll::SpscQueue` | **SHIPPED** | tests + examples |
| Boost.Lockfree / moodycamel | **SHIPPED** / **OPTIONAL** | industry suite |
| Memory order tutorial | **SHIPPED** | `examples/memory_order` |

---

## 5. Compiler & language

| Item | Status | Evidence |
|------|--------|----------|
| C++26 project | **SHIPPED** | CMake |
| CRTP / likely | **SHIPPED** | `ll/branch.hpp` |
| LTO/PGO guidance | **DOC** | `05-compiler.md` |

---

## 6. Benchmarking & telemetry

| Item | Status | Evidence |
|------|--------|----------|
| TSC / steady_ns | **SHIPPED** | `ll/tsc_clock.hpp` |
| Portable HDR histogram | **SHIPPED** | `ll/HdrLatencyHistogram` |
| **HdrHistogram_c** | **SHIPPED** (FetchContent) | `ll/hdr_c.hpp`, `[roadmap][hdr_c]` |
| Google Benchmark | **OPTIONAL** | `bench_queues` |

---

## Focus summary

| Rank | Layer | Why |
|------|--------|-----|
| 1 | §4 + §3 | Queues, arenas, pmr |
| 2 | Library mesh + industry optional | Asio… + hwloc/FB/mimalloc/moodycamel |
| 3 | §6 Telemetry | Portable HDR + **HdrHistogram_c** |
| 4 | §2 Serde + §1 NUMA | **SBE codegen**, numa/uring APIs |
| 5 | §2 Bypass drivers | Contract + lab; DPDK/Onload not linked |

### Roadmap status (this release)

| # | Item | Status |
|---|------|--------|
| 1 | Real Logic **SBE codegen** | **DONE** — schema, generator script, committed headers, tests, tutorial |
| 2 | Linux **numa + uring** + CI | **DONE** — APIs, tests, `linux-roadmap` CI job |
| 3 | **HdrHistogram_c** wrapper | **DONE** — FetchContent + `ll::HdrHistogramC` |
| 4 | **DPDK / Onload lab** | **DONE** as contract+stub+runbook (driver **GAP** by design) |

This audit is the checklist for PRs: new work should move a row from **DOC/GAP → SHIPPED/OPTIONAL** with tests.
