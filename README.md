# C++26 Systems Stack

Buildable C++26 integration stack for systems programming: async I/O, parallelism,
JSON/RPC, portable low-latency primitives (`include/ll/`), and optional industry
libraries. CMake 3.28+, Catch2/GTest, examples, and documentation under `docs/`.

| | |
|--|--|
| Language | C++26 |
| Build | CMake ≥ 3.28, Make, optional Ninja / CMake Presets |
| Platforms | macOS (arm64 / x86_64), Linux (x86_64 / aarch64) |
| License | [MIT](LICENSE) |
| Monorepo | Submodule of [cpp_agents_benchmark](https://github.com/Dmdv/cpp_agents_benchmark) as `systems_stack/` |

---

## Contents

1. [Quick start](#quick-start)
2. [What this repository provides](#what-this-repository-provides)
3. [Documentation index](#documentation-index)
4. [Repository layout](#repository-layout)
5. [Components](#components)
6. [Dependencies](#dependencies)
7. [Build](#build)
8. [Tests and examples](#tests-and-examples)
9. [Design notes](#design-notes)
10. [Related repositories](#related-repositories)
11. [License](#license)

---

## Quick start

```bash
# Dependencies (macOS / Homebrew)
brew install cmake ninja fmt spdlog tbb asio boost taskflow \
  nlohmann-json simdjson eigen protobuf grpc range-v3 abseil catch2 googletest
make install-industry   # hwloc flatbuffers google-benchmark mimalloc jemalloc

# Build and test
make                    # or: make ninja
make run                # ./build/lib_smoke
make examples
```

CLion: open the repo root, enable preset `clion-debug`, build target `ide_index`.  
See [docs/clion.md](docs/clion.md).

---

## What this repository provides

| Area | Description |
|------|-------------|
| Library mesh | Asio, Beast, Taskflow, oneTBB, stdexec, simdjson, protobuf/gRPC, fmt, spdlog, Abseil, Eigen, range-v3 |
| Low-latency core | Header-only `ll::*`: SPSC, arenas, pmr, HDR, SBE, affinity, TSC, NUMA/uring APIs, bypass contract |
| Industry optional | hwloc, FlatBuffers, struct_pack, moodycamel, Folly PCQ, mimalloc, jemalloc, HdrHistogram_c, Google Benchmark; DPDK/Onload detect when installed |
| Validation | `lib_smoke`, Catch2/GTest suites, examples, optional microbenchmarks |
| Docs | Six-layer blueprint, industry audit map, tutorials (build, tooling, SBE, numa/uring, bypass) |

**Focus:** concurrency and memory primitives plus a modern C++ library mesh.  
Kernel-bypass **drivers** (DPDK / OpenOnload) are detected when present; CI uses a stub RX path. Full status: [docs/blueprint/AUDIT.md](docs/blueprint/AUDIT.md).

---

## Documentation index

### Blueprint (low-latency layers)

| Doc | Topic |
|-----|--------|
| [docs/blueprint/README.md](docs/blueprint/README.md) | Blueprint overview and layer index |
| [docs/blueprint/LOW_LATENCY_STACK.md](docs/blueprint/LOW_LATENCY_STACK.md) | Stack narrative |
| [docs/blueprint/AUDIT.md](docs/blueprint/AUDIT.md) | Checklist: SHIPPED / OPTIONAL / GAP |
| [docs/blueprint/01-hardware-os.md](docs/blueprint/01-hardware-os.md) | §1 Hardware & OS (pinning, NUMA, isolation) |
| [docs/blueprint/02-network-ingress.md](docs/blueprint/02-network-ingress.md) | §2 Network ingress & encoding |
| [docs/blueprint/03-memory.md](docs/blueprint/03-memory.md) | §3 Memory & cache locality |
| [docs/blueprint/04-concurrency.md](docs/blueprint/04-concurrency.md) | §4 Lock-free concurrency |
| [docs/blueprint/05-compiler.md](docs/blueprint/05-compiler.md) | §5 Compiler & language |
| [docs/blueprint/06-telemetry.md](docs/blueprint/06-telemetry.md) | §6 Benchmarking & telemetry |
| [docs/blueprint/07-industry-libraries.md](docs/blueprint/07-industry-libraries.md) | Industry library map by layer |

### Tutorials

| Doc | Topic |
|-----|--------|
| [docs/tutorials/industry-stack.md](docs/tutorials/industry-stack.md) | Industry libraries on each layer (install & run) |
| [docs/tutorials/sbe-codegen.md](docs/tutorials/sbe-codegen.md) | Real Logic SBE schema → C++ codecs |
| [docs/tutorials/linux-numa-uring.md](docs/tutorials/linux-numa-uring.md) | Linux NUMA + io_uring |
| [docs/tutorials/hdrhistogram-c.md](docs/tutorials/hdrhistogram-c.md) | Portable HDR and HdrHistogram_c |
| [docs/tutorials/kernel-bypass-lab.md](docs/tutorials/kernel-bypass-lab.md) | DPDK / OpenOnload lab (contract + stub) |
| [docs/tutorials/ninja-build.md](docs/tutorials/ninja-build.md) | Building with Ninja |
| [docs/tutorials/modern-cpp-tooling-arm-intel.md](docs/tutorials/modern-cpp-tooling-arm-intel.md) | Compilers, flags, ARM & Intel, cross-compilation |

### Library guides

| Doc | Component |
|-----|-----------|
| [docs/libraries/README.md](docs/libraries/README.md) | Library index and selection notes |
| [docs/libraries/asio.md](docs/libraries/asio.md) | Asio |
| [docs/libraries/beast.md](docs/libraries/beast.md) | Boost.Beast |
| [docs/libraries/taskflow.md](docs/libraries/taskflow.md) | Taskflow |
| [docs/libraries/tbb.md](docs/libraries/tbb.md) | oneTBB |
| [docs/libraries/stdexec.md](docs/libraries/stdexec.md) | stdexec (P2300) |
| [docs/libraries/simdjson.md](docs/libraries/simdjson.md) | simdjson |
| [docs/libraries/grpc-protobuf.md](docs/libraries/grpc-protobuf.md) | gRPC + Protocol Buffers |
| [docs/libraries/folly.md](docs/libraries/folly.md) | Folly (optional) |
| [docs/libraries/hpx.md](docs/libraries/hpx.md) | HPX (optional) |

### IDE, toolchains, CI, wiki

| Doc | Topic |
|-----|--------|
| [docs/clion.md](docs/clion.md) | CLion presets, IntelliSense (`ide_index`) |
| [cmake/toolchains/README.md](cmake/toolchains/README.md) | Example CMake toolchain files (ARM / x86_64 / macOS) |
| [CMakePresets.json](CMakePresets.json) | `clion-debug`, `default`, Ninja presets |
| [.github/workflows/ci.yml](.github/workflows/ci.yml) | macOS full stack + Linux roadmap smoke |
| [GitHub Wiki](https://github.com/Dmdv/cpp26-systems-stack/wiki) | Published docs mirror (see [docs/WIKI.md](docs/WIKI.md)) |

---

## Repository layout

```text
.
├── CMakeLists.txt
├── CMakePresets.json
├── Makefile
├── include/ll/                 # portable low-latency headers
├── generated/sbe/              # committed SBE C++ codecs
├── schemas/                    # FlatBuffers + SBE XML
├── src/                        # lib_smoke integration binary
├── examples/                   # demos (spsc, pmr, sbe, bypass, …)
├── benchmarks/                 # Google Benchmark targets
├── tests/                      # Catch2 + GTest
├── tools/ide_index.cpp         # CLion / clangd index TU
├── scripts/                    # SBE generator, Linux CI smoke CMake
├── cmake/toolchains/           # cross-compile examples
└── docs/
    ├── blueprint/              # layers 01–07, AUDIT, narrative
    ├── libraries/              # per-library guides
    ├── tutorials/              # hands-on how-tos
    └── clion.md
```

---

## Components

### Library mesh

| Domain | Component | Guide | Tests |
|--------|-----------|-------|-------|
| Async I/O | Asio (standalone) | [asio.md](docs/libraries/asio.md) | `test_beast_asio` |
| HTTP / WebSocket | Boost.Beast | [beast.md](docs/libraries/beast.md) | `test_beast_asio` |
| Task graphs | Taskflow | [taskflow.md](docs/libraries/taskflow.md) | `test_taskflow` |
| Data parallel | oneTBB | [tbb.md](docs/libraries/tbb.md) | `test_libs` |
| Senders / receivers | stdexec | [stdexec.md](docs/libraries/stdexec.md) | `test_libs` |
| JSON | simdjson, nlohmann-json | [simdjson.md](docs/libraries/simdjson.md) | `test_libs` |
| Schema / RPC | protobuf, gRPC | [grpc-protobuf.md](docs/libraries/grpc-protobuf.md) | `test_libs` |
| Optional services | Folly, HPX | [folly.md](docs/libraries/folly.md), [hpx.md](docs/libraries/hpx.md) | `make folly` / `make hpx` |
| Utilities | fmt, spdlog, Abseil, range-v3, Eigen | — | `test_libs` |

### Low-latency modules (`include/ll/`)

| Header | Role | Layer |
|--------|------|-------|
| `ll/spsc_queue.hpp` | Lock-free SPSC ring | §4 |
| `ll/arena.hpp`, `ll/pmr_arena.hpp` | Arena / object pool / std::pmr | §3 |
| `ll/cache_line.hpp` | Cache-line constants / padding | §3 |
| `ll/sbe_style.hpp`, `ll/sbe_codec.hpp` | SBE-style POD + generated codecs | §2 |
| `ll/struct_pack_tick.hpp` | struct_pack tick helpers | §2 |
| `ll/affinity.hpp` | Thread pin / macOS QoS | §1 |
| `ll/linux_numa.hpp`, `ll/linux_uring.hpp` | NUMA / io_uring (Linux or stub) | §1–2 |
| `ll/kernel_bypass.hpp` | Poll-mode RX contract + stub | §2 |
| `ll/tsc_clock.hpp`, `ll/hdr_histogram.hpp`, `ll/hdr_c.hpp` | Timestamps / HDR | §6 |
| `ll/branch.hpp` | likely/unlikely, CRTP helper | §5 |
| `ll/jemalloc_util.hpp` | Optional jemalloc helpers | §3 |
| `ll/industry.hpp` | Umbrella + `LL_HAS_*` macros | — |

### Industry libraries (CMake auto-detect)

| Library | Role | Enable |
|---------|------|--------|
| Boost.Lockfree | SPSC queue | Always (Boost required) |
| moodycamel | SPSC (FetchContent) | `STACK_WITH_MOODYCAMEL` |
| hwloc | Topology | `brew install hwloc` |
| FlatBuffers | Zero-copy tables | `brew install flatbuffers` |
| struct_pack | Compile-time pack | FetchContent (yalantinglibs) |
| mimalloc / jemalloc | Off-path allocators | `make install-industry` |
| Folly PCQ | SPSC when Folly present | `brew install folly` |
| HdrHistogram_c | Tail latency | FetchContent |
| Google Benchmark | Microbenchmarks | `brew install google-benchmark` |
| libnuma / liburing | Linux only | distro packages |
| DPDK / OpenOnload | Header detect if installed | lab NIC; stub otherwise |

Full map: [docs/blueprint/07-industry-libraries.md](docs/blueprint/07-industry-libraries.md).

---

## Dependencies

### Base (Homebrew)

```bash
brew install cmake ninja fmt spdlog tbb asio boost taskflow \
  nlohmann-json simdjson eigen protobuf grpc \
  range-v3 abseil catch2 googletest
```

### Industry optional

```bash
make install-industry     # hwloc flatbuffers google-benchmark mimalloc jemalloc
brew install folly        # optional: ProducerConsumerQueue tests
# Linux: libnuma-dev liburing-dev
```

### Optional heavy runtimes

```bash
brew install folly
make install-hpx          # local prefix, default ~/cpp-deps/hpx
```

```bash
make deps-check           # inventory installed packages
```

---

## Build

| Goal | Command |
|------|---------|
| Configure, build, test (default tree) | `make` |
| Ninja tree (`build_ninja/`) | `make ninja` |
| Run integration binary | `make run` |
| Examples | `make examples` |
| Microbenchmarks | `make bench` |
| Folly / HPX / both | `make folly` · `make hpx` · `make full` |
| CLion debug + index | `make clion` · `make clion-index` |
| Clean all build trees | `make distclean` |

```bash
# Explicit CMake
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix)"
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Optional CMake flags: `LIB_SMOKE_WITH_FOLLY`, `LIB_SMOKE_WITH_HPX`, `STACK_WITH_*`  
(see `CMakeLists.txt` and `cmake --help-variable-cache` after configure).

More detail:

- Ninja: [docs/tutorials/ninja-build.md](docs/tutorials/ninja-build.md)
- ARM / Intel / cross: [docs/tutorials/modern-cpp-tooling-arm-intel.md](docs/tutorials/modern-cpp-tooling-arm-intel.md)
- CLion: [docs/clion.md](docs/clion.md)

---

## Tests and examples

### Suites

| Target | Framework | Focus |
|--------|-----------|--------|
| `lib_smoke` | binary | End-to-end library checks |
| `test_beast_asio` | Catch2 | Asio / Beast |
| `test_taskflow` | Catch2 | Task graphs |
| `test_libs` | Catch2 | Mesh libraries |
| `test_ll_modules` | Catch2 | `ll` primitives |
| `test_industry_stack` | Catch2 | Industry deps + serde + queues |
| `test_roadmap_stack` | Catch2 | SBE, numa/uring, HDR_c, bypass |
| `test_gtest_smoke` | GTest | Framework smoke |
| `test_folly` / `test_hpx` | Catch2 | Optional profiles |

```bash
ctest --test-dir build --output-on-failure
ctest --test-dir build -R 'll|industry|roadmap'
```

### Example binaries

| Binary | Topic |
|--------|--------|
| `example_spsc` | Lock-free SPSC handoff |
| `example_arena` / `example_pmr` | Bump / pmr arenas |
| `example_memory_order` | acquire/release handoff |
| `example_tsc` / `example_hdr` / `example_hdr_c` | Timestamps and histograms |
| `example_sbe_style` / `example_sbe_codegen` | POD wire vs generated SBE |
| `example_struct_pack` | yalantinglibs struct_pack |
| `example_industry_queues` | ll / Boost / moodycamel |
| `example_numa_uring` | NUMA + io_uring status |
| `example_kernel_bypass` | Stub poll-mode RX + SBE |
| `bench_queues` | Google Benchmark |

```bash
make examples
make bench
```

---

## Design notes

1. C++26 as the project standard; industry libraries opt in via CMake.
2. Separate concerns: I/O, parse, schedule, compute, RPC, and hot-path primitives.
3. Prefer proven queues and allocators over reimplementing them on the critical path.
4. Auto-detect optional packages; keep the default configure path usable without Folly/HPX/DPDK.
5. Prefer p99 / p99.9 / max over mean latency for anything timing-sensitive.
6. Keep the hot-path instruction footprint small (i-cache); see blueprint §5.

---

## Related repositories

| Repository | Role |
|------------|------|
| [cpp26-systems-stack](https://github.com/Dmdv/cpp26-systems-stack) | This project |
| [hft-asm-l2-conflator](https://github.com/Dmdv/hft-asm-l2-conflator) | AArch64 assembler L2 conflator |
| [cpp_agents_benchmark](https://github.com/Dmdv/cpp_agents_benchmark) | Private monorepo (gitlinks only) |

```bash
git clone --recurse-submodules https://github.com/Dmdv/cpp_agents_benchmark.git
```

---

## License

MIT — see [LICENSE](LICENSE).
