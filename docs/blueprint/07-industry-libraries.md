# Industry libraries mapped to the six-layer stack

Companion to the review checklist. Status: **SHIPPED** (code+test), **OPTIONAL**
(auto-on when installed), **DOC**, or **GAP**.

| Layer | Library | Status | Evidence |
|-------|---------|--------|----------|
| §1 HW/OS | **hwloc** | **OPTIONAL** | `ctest -R hwloc`, `STACK_WITH_HWLOC` |
| §1 HW/OS | **libnuma / numactl** | **SHIPPED** API · Linux **OPTIONAL** link | `ll/linux_numa.hpp` |
| §2 Ingress | **liburing** | **SHIPPED** API · Linux **OPTIONAL** link | `ll/linux_uring.hpp` |
| §2 Ingress | **DPDK** | **OPTIONAL detect** + stub RX | `STACK_WITH_DPDK`, `ll/kernel_bypass.hpp` |
| §2 Ingress | **OpenOnload / EF_VI** | **OPTIONAL detect** + stub RX | `STACK_WITH_ONLOAD` |
| §2 Serde | **Real Logic SBE** | **SHIPPED** | `generated/sbe/`, `ll/sbe_codec.hpp` |
| §2 Serde | SBE-style POD | **SHIPPED** | `ll/sbe_style.hpp` |
| §2 Serde | **FlatBuffers** | **OPTIONAL** | `schemas/tick.fbs` |
| §2 Serde | **struct_pack** | **SHIPPED** (FetchContent) | `ll/struct_pack_tick.hpp`, yalantinglibs |
| §3 Memory | **std::pmr** | **SHIPPED** | `ll/pmr_arena.hpp` |
| §3 Memory | **mimalloc** | **OPTIONAL** | industry tests |
| §3 Memory | **jemalloc** | **OPTIONAL** | `ll/jemalloc_util.hpp` |
| §4 Queues | **ll::SpscQueue** | **SHIPPED** | teaching + tiny footprint |
| §4 Queues | **moodycamel::ReaderWriterQueue** | **OPTIONAL** | FetchContent |
| §4 Queues | **Boost.Lockfree spsc_queue** | **SHIPPED** | always (Boost required) |
| §4 Queues | **folly::ProducerConsumerQueue** | **OPTIONAL** | auto if Folly installed; also `make folly` |
| §6 Telemetry | **ll::HdrLatencyHistogram** | **SHIPPED** | portable log buckets |
| §6 Telemetry | **HdrHistogram_c** | **SHIPPED** | FetchContent + `ll::HdrHistogramC` |
| §6 Telemetry | **Google Benchmark** | **OPTIONAL** | `bench_queues` |

## Selection guidance

1. **Default portable path:** `ll::*` + Boost.Lockfree + std::pmr + Asio/Beast + SBE + struct_pack.  
2. **Add when available:** hwloc, FlatBuffers, moodycamel, mimalloc, jemalloc, Google Benchmark, Folly PCQ.  
3. **Linux production envelope:** numa, uring, isolcpus, hugepages.  
4. **Venue-facing wire:** Real Logic SBE and/or struct_pack — not JSON on the hot path.  
5. **Kernel bypass:** detect DPDK/Onload headers when present; implement `IPollModeRx` on lab NIC.

Tutorials: [`docs/tutorials/industry-stack.md`](../tutorials/industry-stack.md).
