# Industry libraries mapped to the six-layer stack

Companion to the review checklist. Status: **SHIPPED** (code+test), **OPTIONAL**
(auto-on when installed), **DOC**, or **GAP**.

| Layer | Library | Status | Evidence |
|-------|---------|--------|----------|
| §1 HW/OS | **hwloc** | **OPTIONAL** | `ctest -R hwloc`, CMake `STACK_WITH_HWLOC` |
| §1 HW/OS | **libnuma / numactl** | **DOC** / Linux **OPTIONAL** | `01-hardware-os.md`, soft find |
| §2 Ingress | **DPDK** | **GAP** | driver product — not vendored |
| §2 Ingress | **OpenOnload / libef_vi** | **GAP** | Solarflare SDK |
| §2 Ingress | **liburing** | Linux **OPTIONAL** | soft find `liburing` |
| §2 Serde | **Real Logic SBE** | **PARTIAL** | `ll/sbe_style.hpp` + tutorial (use official codegen in prod) |
| §2 Serde | **FlatBuffers** | **OPTIONAL** | `schemas/tick.fbs`, `[industry][flatbuffers]` |
| §2 Serde | **struct_pack** | **DOC** | faster-than-protobuf alternative; not vendored |
| §3 Memory | **std::pmr** | **SHIPPED** | `ll/pmr_arena.hpp`, `[industry][pmr]` |
| §3 Memory | **mimalloc** | **OPTIONAL** | off-path allocator smoke |
| §3 Memory | **jemalloc** | **DOC** | install alongside mimalloc guidance |
| §4 Queues | **ll::SpscQueue** | **SHIPPED** | teaching + tiny footprint |
| §4 Queues | **moodycamel::ReaderWriterQueue** | **OPTIONAL** | FetchContent v1.0.6 |
| §4 Queues | **Boost.Lockfree spsc_queue** | **SHIPPED** | always (Boost required) |
| §4 Queues | **folly::ProducerConsumerQueue** | **OPTIONAL** | `LIB_SMOKE_WITH_FOLLY` |
| §6 Telemetry | **ll::HdrLatencyHistogram** | **SHIPPED** | portable log buckets |
| §6 Telemetry | **HdrHistogram_c** | **DOC** | production-grade; integrate when needed |
| §6 Telemetry | **Google Benchmark** | **OPTIONAL** | `bench_queues` |

## Selection guidance

1. **Default portable path:** `ll::*` + Boost.Lockfree + std::pmr + Asio/Beast.  
2. **Add when available:** hwloc, FlatBuffers, moodycamel, Google Benchmark, mimalloc.  
3. **Linux production envelope:** numa, uring, isolcpus, hugepages (docs).  
4. **Venue-facing wire:** Real Logic SBE (generated), not JSON.  
5. **Kernel bypass:** separate lab/repo with NIC ownership — not this monorepo’s default CI.

Full tutorial: [`docs/tutorials/industry-stack.md`](../tutorials/industry-stack.md).
