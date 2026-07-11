# C++26 Systems Stack

[![C++26](https://img.shields.io/badge/C%2B%2B-26-blue.svg)](#language--toolchain)
[![CMake](https://img.shields.io/badge/CMake-3.28%2B-green.svg)](#build)
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux-lightgrey.svg)](#dependencies)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**Integration laboratory for a modern C++26 systems ecosystem** — async I/O, HTTP/WebSocket, task graphs, data-parallel runtimes, sender/receiver composition, high-performance JSON, schema/RPC, and optional service-side foundations (Folly, HPX).

This repository is **not** a toy “hello world” collection. It is a **buildable reference stack**: CMake-wired dependencies, executable integration binary, Catch2/GTest suites, and architecture-oriented library guides under [`docs/libraries/`](docs/libraries/).

| | |
|--|--|
| **Language** | ISO C++26 (`CMAKE_CXX_STANDARD 26`) |
| **Build** | CMake 3.28+ · Make wrapper |
| **Package sources** | Homebrew (base) · FetchContent (`stdexec`) · optional local HPX |
| **Monorepo** | Submodule of private [`Dmdv/cpp_agents_benchmark`](https://github.com/Dmdv/cpp_agents_benchmark) as `systems_stack/` |

---

## Ecosystem map

```mermaid
flowchart TB
  subgraph IO["I/O & wire protocols"]
    ASIO[Asio · timers / sockets / executors]
    BEAST[Boost.Beast · HTTP / WebSocket]
    GRPC[gRPC · RPC transport]
    PB[Protocol Buffers · schemas]
  end

  subgraph PARSE["Parsing & data"]
    SIMD[simdjson · DOM / on-demand]
    NLOH[nlohmann-json · ergonomic JSON]
    EIGEN[Eigen · dense linear algebra]
  end

  subgraph EXEC["Execution & parallelism"]
    TF[Taskflow · explicit DAGs]
    TBB[oneTBB · algorithms / containers]
    STDEX[stdexec · P2300 senders/receivers]
    HPX[HPX · optional parallel runtime]
  end

  subgraph UTIL["Foundation utilities"]
    FMT[fmt · formatting]
    SPD[spdlog · logging]
    ABSL[Abseil · strings / containers]
    RV[range-v3 · views]
    FOLLY[Folly · optional service utilities]
  end

  BEAST --> ASIO
  GRPC --> PB
  ASIO --> PARSE
  BEAST --> PARSE
  PARSE --> EXEC
  EXEC --> UTIL
```

### Suggested composition for a market-data / services path

```mermaid
flowchart LR
  W[WebSocket / HTTP<br/>Beast + Asio] --> J[JSON ticks<br/>simdjson]
  W --> S[Internal messages<br/>protobuf]
  J --> P[Pipeline stages<br/>Taskflow]
  S --> P
  P --> B[Batch compute<br/>TBB / Eigen]
  P --> R[RPC egress<br/>gRPC]
  P --> C[In-process cache<br/>F14 / Abseil]
```

---

## Repository layout

```text
.
├── CMakeLists.txt          # C++26 project · optional Folly/HPX
├── Makefile                # developer workflows (base / folly / hpx / full)
├── proto/smoke.proto       # sample protobuf message
├── src/main.cpp            # integration binary: one-shot stack exercise
├── src/folly_compat.hpp    # Folly + libc++ coexistence helpers
├── tests/                  # Catch2 + GTest integration suites
├── scripts/install_hpx.sh  # local HPX bootstrap (not on Homebrew)
└── docs/libraries/         # per-component architecture notes
```

---

## Component catalog

| Domain | Component | Guide | Validation |
|--------|-----------|-------|------------|
| Async I/O | **Asio** (standalone) | [asio.md](docs/libraries/asio.md) | `test_beast_asio` |
| HTTP / WS | **Boost.Beast** | [beast.md](docs/libraries/beast.md) | `ctest -R beast` |
| Task DAGs | **Taskflow** | [taskflow.md](docs/libraries/taskflow.md) | `test_taskflow` |
| Data parallel | **oneTBB** | [tbb.md](docs/libraries/tbb.md) | `ctest -R tbb` / `test_libs` |
| Senders / receivers | **stdexec** (P2300) | [stdexec.md](docs/libraries/stdexec.md) | `ctest -R stdexec` / `test_libs` |
| JSON (hot path) | **simdjson** | [simdjson.md](docs/libraries/simdjson.md) | `ctest -R simdjson` |
| Schema / RPC | **protobuf + gRPC** | [grpc-protobuf.md](docs/libraries/grpc-protobuf.md) | `ctest -R protobuf` |
| Service utilities | **Folly** (optional) | [folly.md](docs/libraries/folly.md) | `make folly` |
| Parallel runtime | **HPX** (optional) | [hpx.md](docs/libraries/hpx.md) | `make hpx` |
| Formatting / log | **fmt**, **spdlog** | catalog below | `test_libs` |
| Strings / maps | **Abseil** | catalog below | `test_libs` |
| Views | **range-v3** | catalog below | `test_libs` |
| Linear algebra | **Eigen** | catalog below | `test_libs` |
| Unit frameworks | **Catch2**, **GTest** | — | `ctest` |

Full index and selection matrix: [`docs/libraries/README.md`](docs/libraries/README.md).

---

## Language & toolchain

| Requirement | Value |
|-------------|--------|
| ISO C++ | **26** (`CMAKE_CXX_STANDARD 26`, extensions off) |
| CMake | **≥ 3.28** |
| Compilers | Recent Clang (Apple/LLVM) or GCC with C++26 support |
| Package manager (macOS) | Homebrew under `/opt/homebrew` or `/usr/local` |

Design choices encoded in CMake:

- **Base profile** configures quickly without Folly/HPX  
- **Folly / HPX** are explicit opt-in (`LIB_SMOKE_WITH_FOLLY`, `LIB_SMOKE_WITH_HPX`)  
- **stdexec** is pulled via **FetchContent** (not packaged on Homebrew)  
- Folly is deliberately **not** on the shared interface target to avoid include-flag collisions with Beast/Asio on Apple Clang

---

## Dependencies

### Base stack (Homebrew)

```bash
brew install cmake fmt spdlog tbb asio boost taskflow \
  nlohmann-json simdjson eigen protobuf grpc \
  range-v3 abseil catch2 googletest
```

### Optional

```bash
brew install folly
make install-hpx          # local prefix, default ~/cpp-deps/hpx
```

### Status check

```bash
make deps-check
```

---

## Build

```bash
# Base stack: configure + build + ctest
make

# Optional profiles (isolated build directories)
make folly                # + Folly
make hpx                  # + HPX
make full                 # Folly + HPX

# Integration binary
make run

# IDE / clangd
make compile-commands
```

| Make target | Effect |
|-------------|--------|
| `make` / `all` | Base configure, build, test |
| `make run` | Execute integration binary |
| `make test` | `ctest` in current `BUILD_DIR` |
| `make folly` / `hpx` / `full` | Optional stacks |
| `make distclean` | Remove all `build*` trees |
| `make deps-check` | Inventory Homebrew + HPX |

CMake options:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DLIB_SMOKE_WITH_FOLLY=ON \
  -DLIB_SMOKE_WITH_HPX=ON \
  -DLIB_SMOKE_HPX_ROOT=$HOME/cpp-deps/hpx
```

---

## Samples & tests

### Integration binary (`src/main.cpp`)

One process walks the stack end-to-end and prints `[PASS]` / `[FAIL]` per check: Asio, Beast version, fmt/spdlog, Abseil, TBB reduce, Taskflow, ranges, JSON (nlohmann + simdjson), Eigen, protobuf/gRPC versions, stdexec pool, and optionally Folly/HPX.

```bash
./build/lib_smoke
```

### Automated suites

| Target | Framework | Focus |
|--------|-----------|--------|
| `test_beast_asio` | Catch2 | Asio posts/timers · Beast HTTP types |
| `test_taskflow` | Catch2 | Task graph execution |
| `test_libs` | Catch2 | fmt, TBB, simdjson, nlohmann, Eigen, Abseil, ranges, protobuf, stdexec |
| `test_gtest_smoke` | GTest | Framework install sanity |
| `test_folly` | Catch2 | Folly (optional profile) |
| `test_hpx` | Catch2 | HPX (optional profile) |

```bash
ctest --test-dir build --output-on-failure
ctest --test-dir build -R simdjson
```

---

## Design principles

1. **C++26 first** — standard-library direction (senders/receivers, jthread, etc.) coexists with industry libs.  
2. **Composable layers** — I/O, parse, schedule, compute, RPC are separate concerns.  
3. **Measurable integration** — every package has either a unit in `test_libs` or a dedicated suite.  
4. **Optional heavyweights** — Folly/HPX do not tax the default configure path.  
5. **Production-minded wiring** — real `find_package`, protobuf generation, and known macOS Folly/Beast coexistence constraints documented in CMake.

---

## Monorepo integration

Private suite: [`Dmdv/cpp_agents_benchmark`](https://github.com/Dmdv/cpp_agents_benchmark)

```text
cpp_agents_benchmark/          (private)
├── systems_stack/  ──submodule──►  Dmdv/cpp26-systems-stack  (this repo)
├── asm_test/       ──submodule──►  Dmdv/hft-asm-l2-conflator
└── … agent benchmarks …
```

```bash
git clone --recurse-submodules https://github.com/Dmdv/cpp_agents_benchmark.git
```

Sources for this stack live **only** here; the private monorepo holds a gitlink, not a second copy.

---

## Related repositories

| Repo | Role |
|------|------|
| [cpp26-systems-stack](https://github.com/Dmdv/cpp26-systems-stack) | This project — C++26 systems ecosystem |
| [hft-asm-l2-conflator](https://github.com/Dmdv/hft-asm-l2-conflator) | AArch64 assembler HFT L2 conflator |
| [cpp_agents_benchmark](https://github.com/Dmdv/cpp_agents_benchmark) | Private multi-agent benchmark monorepo |

---

## License

MIT — see [LICENSE](LICENSE).
