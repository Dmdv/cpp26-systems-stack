# Industry libraries mapped to the six-layer stack

Companion to the review checklist. Status: **SHIPPED** (code+test), **OPTIONAL**
(auto-on when installed), **DOC**, or **GAP**.

| Layer | Library | Status | Evidence |
|-------|---------|--------|----------|
| §1 HW/OS | **hwloc** | **OPTIONAL** | `ctest -R hwloc`, CMake `STACK_WITH_HWLOC` |
| §1 HW/OS | **libnuma / numactl** | **SHIPPED** API · Linux **OPTIONAL** link | `ll/linux_numa.hpp`, Linux CI |
| §2 Ingress | **liburing** | **SHIPPED** API · Linux **OPTIONAL** link | `ll/linux_uring.hpp`, Linux CI |
| §2 Ingress | **DPDK / OpenOnload / EF_VI** | **GAP** + **lab contract** | `ll/kernel_bypass.hpp`, kernel-bypass tutorial |
| §2 Serde | **Real Logic SBE** | **SHIPPED** | schema + `generated/sbe/` + `ll/sbe_codec.hpp` |
| §2 Serde | SBE-style POD | **SHIPPED** | `ll/sbe_style.hpp` |
| §2 Serde | **FlatBuffers** | **OPTIONAL** | `schemas/tick.fbs` |
| §2 Serde | **struct_pack** | **DOC** | not vendored |
| §3 Memory | **std::pmr** | **SHIPPED** | `ll/pmr_arena.hpp` |
| §3 Memory | **mimalloc** | **OPTIONAL** | industry tests |
| §3 Memory | **jemalloc** | **DOC** | install guidance |
| §4 Queues | **ll::SpscQueue** | **SHIPPED** | teaching + tiny footprint |
| §4 Queues | **moodycamel::ReaderWriterQueue** | **OPTIONAL** | FetchContent |
| §4 Queues | **Boost.Lockfree spsc_queue** | **SHIPPED** | always (Boost required) |
| §4 Queues | **folly::ProducerConsumerQueue** | **OPTIONAL** | Folly profile |
| §6 Telemetry | **ll::HdrLatencyHistogram** | **SHIPPED** | portable log buckets |
| §6 Telemetry | **HdrHistogram_c** | **SHIPPED** | FetchContent + `ll::HdrHistogramC` |
| §6 Telemetry | **Google Benchmark** | **OPTIONAL** | `bench_queues` |

## Selection guidance

1. **Default portable path:** `ll::*` + Boost.Lockfree + std::pmr + Asio/Beast + SBE codecs.  
2. **Add when available:** hwloc, FlatBuffers, moodycamel, Google Benchmark, mimalloc.  
3. **Linux production envelope:** numa, uring, isolcpus, hugepages.  
4. **Venue-facing wire:** Real Logic SBE (generated), not JSON.  
5. **Kernel bypass:** implement `IPollModeRx` on lab NIC hardware (runbook).

Tutorials: [`docs/tutorials/`](../tutorials/).
