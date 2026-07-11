# Tutorial: Modern C++ tooling — compilation, cross-compilation, ARM & Intel

This guide explains how **modern C++ toolchains** choose compilers, ISAs, and
linkers when you target **ARM (AArch64)** and **Intel/AMD (x86-64)**. It is
written for this repository (`cpp26-systems-stack`: CMake 3.28+, C++26, Ninja
optional) but the patterns apply to any CMake-based systems project.

> **Related:** [ninja-build.md](ninja-build.md) (build system) ·  
> [docs/clion.md](../clion.md) (IDE) · [05-compiler.md](../blueprint/05-compiler.md)  
> (constexpr, LTO, i-cache discipline).

---

## 0. Mental model

```text
  Language dialect     C++26  (-std=c++26)
         │
  Compiler frontend    Clang / GCC / MSVC
         │
  Codegen backend      AArch64  |  x86-64   (+ optional -march / -mtune)
         │
  Object files         .o / .obj
         │
  Linker               lld / ld64 / gold / link.exe
         │
  Binary               macho / ELF / PE   for a *specific* OS + CPU
```

| Layer | Tool examples | What it decides |
|-------|----------------|-----------------|
| Meta-build | **CMake** | Targets, flags, find packages, toolchain file |
| Build graph | **Ninja** / Make | What to recompile |
| Compiler | **Clang**, **GCC**, MSVC | Parse C++, emit machine code for an ISA |
| ISA / µarch | `-march=`, `-mcpu=`, `/arch:` | Which instructions are legal |
| Sanitizers / LTO | `-fsanitize=`, `-flto` | Correctness & whole-program opts |

**Cross-compilation** means: the machine that *runs the compiler* (build host)
differs from the machine that *runs the binary* (target). Example: compile on
Apple Silicon for a Linux x86-64 server, or on an Intel laptop for `aarch64-linux-gnu`.

---

## 1. Know your host

```bash
uname -m                    # arm64 | aarch64 | x86_64
uname -s                    # Darwin | Linux
c++ -dumpmachine            # e.g. arm64-apple-darwin25.0.0, x86_64-linux-gnu
clang++ -print-target-triple
cmake -E capabilities | head  # optional
```

| `uname -m` | Typical marketing name | ISA family |
|------------|------------------------|------------|
| `arm64` / `aarch64` | Apple Silicon, Graviton, Ampere, RPi 4/5 64-bit | **AArch64** |
| `x86_64` / `amd64` | Intel Core, AMD Ryzen | **x86-64** |

This repo is regularly built on **Apple arm64** (Clang). The same sources are
portable; only flags and package paths change on Linux x86-64/aarch64.

---

## 2. Modern compiler choice (2024–2026)

### Recommended defaults

| Host | Compiler | Notes |
|------|----------|--------|
| macOS (Apple Silicon or Intel) | **Apple Clang** (Xcode CLT) or Homebrew `llvm` | Project uses C++26 |
| Linux | **GCC 14+** or **Clang 18+** | Check `__cplusplus` / C++26 support |
| Windows | **MSVC 17.10+** or Clang-CL | CMake `MSVC` / `ClangCL` |

```bash
# See what you actually invoke
which c++ clang++ g++
c++ --version
echo | c++ -E -dM - | grep -E '__aarch64__|__x86_64__|__APPLE__|__linux__'
```

### CMake: select compiler *before* first configure

```bash
export CC=clang
export CXX=clang++
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# or explicit (clearest for cross builds)
cmake -S . -B build -G Ninja \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
```

Changing compiler later in the same build tree is error-prone — use a **new**
`-B` directory.

### This project

```cmake
# CMakeLists.txt (already set)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)   # prefer -std=c++26 over -std=gnu++26
```

---

## 3. Architecture flags: portable vs “fast on my laptop”

### Portable baselines (CI, release artifacts)

Prefer **architecture families**, not your exact CPU:

| Target | GCC/Clang flags (examples) | Meaning |
|--------|----------------------------|---------|
| x86-64 “any 64-bit Intel/AMD” | `-march=x86-64` or `-march=x86-64-v2` | SSE2 baseline / v2 ≈ modern servers |
| x86-64 with AVX2 | `-march=x86-64-v3` | Haswell-era+ |
| AArch64 generic | `-march=armv8-a` | Cortex-A53 and up |
| Apple Silicon | often default for host Clang | arm64-apple-darwin |

CMake 3.24+:

```bash
cmake -S . -B build -DCMAKE_CXX_FLAGS_RELEASE="-O3" \
  -DCMAKE_CXX_FLAGS="-march=x86-64-v2"   # example when *targeting* x86-64
```

### Host-tuned (local benchmarks only)

```bash
cmake -S . -B build_native -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native"
```

| Flag | Effect |
|------|--------|
| `-march=native` | Use *this machine’s* ISA extensions (AVX-512, SVE, …) |
| `-mtune=native` | Schedule for this µarch without requiring extra ISA |

**Warning:** `-march=native` binaries may **SIGILL** on older CPUs or different
vendors (e.g. AVX-512 laptop → non-AVX-512 server). Never ship `native` to a
heterogeneous fleet without a fat-binary / dispatch strategy.

### Low-latency note (this stack)

Blueprint layer 5: prefer **portable flags in default CMake**, enable aggressive
ISA flags only in **benchmark / production profiles** you control. Keep the
critical path small enough for L1 i-cache regardless of AVX width.

---

## 4. ARM (AArch64) specifics

### Apple Silicon (arm64-apple-darwin)

```bash
uname -m    # arm64
clang++ -target arm64-apple-macos14 -c hello.cpp
```

- Homebrew packages install under `/opt/homebrew` (arm64) vs `/usr/local` (Intel Mac).
- This project’s Makefile/CMake default `CMAKE_PREFIX_PATH` includes `/opt/homebrew`.
- **Rosetta 2:** you can run *x86_64* binaries on Apple Silicon; building *as*
  x86_64 uses a different triple (see §6).

### Linux aarch64 (Graviton, Ampere, RPi OS 64-bit)

```bash
sudo apt-get install -y g++ cmake ninja-build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Packages are often `libfoo-dev` for aarch64 natively — no cross-compiler needed
when **host == target**.

### NEON / SVE

- **NEON** is essentially baseline on AArch64 (128-bit SIMD).
- **SVE/SVE2** (scalable vectors) need newer cores and explicit `-march=armv8.2-a+sve` (etc.).
- Autovectorization: `-O3` / `-O2` + profile; inspect with `llvm-objdump -d` / `perf`.

Example (illustrative, not required by this repo’s default flags):

```bash
cmake -S . -B build_a64 -G Ninja \
  -DCMAKE_CXX_FLAGS_RELEASE="-O3 -march=armv8-a"
```

---

## 5. Intel / AMD (x86-64) specifics

### Levels (useful mental model)

| Level | Rough contents | CMake/flag sketch |
|-------|----------------|-------------------|
| x86-64 | SSE2 | `-march=x86-64` |
| x86-64-v2 | SSE4.2, POPCNT, … | `-march=x86-64-v2` |
| x86-64-v3 | AVX, AVX2, BMI, … | `-march=x86-64-v3` |
| x86-64-v4 | AVX-512 | `-march=x86-64-v4` |

Detect at runtime (for multi-version code) with `cpuid` / libraries; compile-time
with `#ifdef __AVX2__`.

### Intel vs AMD

`-march=native` on Zen vs Intel may enable different extensions. For **one binary
for both**, pick a conservative `-march=x86-64-v2` or `v3` after checking your
fleet’s minimum CPU.

### Intel compilers (optional)

- **ICX** (LLVM-based oneAPI DPC++/C++) can replace Clang as `CMAKE_CXX_COMPILER`.
- Classic ICC is legacy; prefer ICX for new work.

---

## 6. Cross-compilation patterns

### Vocabulary

| Term | Meaning |
|------|---------|
| **Build** | Machine running CMake/compiler |
| **Host** | Machine that will run *build tools* you produce (often = build) |
| **Target** | Machine that will run your application binary |

CMake uses:

```cmake
CMAKE_SYSTEM_NAME       # Linux, Darwin, Windows
CMAKE_SYSTEM_PROCESSOR  # aarch64, x86_64, arm64, …
CMAKE_C_COMPILER / CMAKE_CXX_COMPILER
CMAKE_FIND_ROOT_PATH    # where target sysroot libraries live
```

### Pattern A — same OS, different CPU (Apple: arm64 host → x86_64 binary)

```bash
# On Apple Silicon, produce an Intel Mac binary (needs Xcode + matching libs)
cmake -S . -B build_x86_on_arm -G Ninja \
  -DCMAKE_OSX_ARCHITECTURES=x86_64 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/usr/local   # Intel Homebrew prefix if installed under Rosetta

cmake --build build_x86_on_arm -j
file build_x86_on_arm/lib_smoke
# expected: Mach-O 64-bit executable x86_64
```

Universal (“fat”) binary:

```bash
cmake -S . -B build_uni -G Ninja \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_BUILD_TYPE=Release
```

**Caveat:** every dependency must provide the same arch slice (painful for
Homebrew unless you maintain dual prefixes). For this stack, **native single-arch**
builds are the practical default.

### Pattern B — Linux: x86_64 host → aarch64 target (true cross)

1. Install cross compiler + sysroot:

```bash
# Debian/Ubuntu example
sudo apt-get install -y \
  gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
  binutils-aarch64-linux-gnu
```

2. Use a **toolchain file** (see `cmake/toolchains/` examples in this repo):

```bash
cmake -S . -B build_cross_a64 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-linux-gnu.cmake \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build_cross_a64 -j
file build_cross_a64/lib_smoke
# ELF 64-bit LSB executable, ARM aarch64
```

3. You still need **target** libraries (fmt, boost, …) for aarch64 — either:
   - multiarch packages / cross sysroot, or  
   - build deps in a **container** for aarch64 (`docker buildx --platform linux/arm64`), or  
   - QEMU user emulation to run aarch64 `apt` in a chroot.

Cross-compiling *this* full mesh on a laptop without a sysroot is intentionally
hard; prefer **native CI runners** per architecture when possible.

### Pattern C — Linux aarch64 host → x86_64 target

Symmetric: `g++-x86-64-linux-gnu` + toolchain file `x86_64-linux-gnu.cmake`.

### Pattern D — Containers / buildx (often easier than classic cross)

```bash
# Build linux/arm64 image on an amd64 machine (buildx + QEMU)
docker buildx build --platform linux/arm64 -t systems-stack:arm64 .
```

Use multi-stage Dockerfiles: install deps *inside* the target platform image so
you do not hand-maintain a sysroot.

### Pattern E — Zig or `clang -target` as a cross driver (advanced)

```bash
# Example sketch only
cmake -S . -B build_zig -G Ninja \
  -DCMAKE_CXX_COMPILER=zig\ c++ \
  -DCMAKE_C_COMPILER=zig\ cc \
  -DCMAKE_CXX_FLAGS="-target aarch64-linux-gnu"
```

Powerful, but package discovery (`find_package`) still needs target libs.

---

## 7. Toolchain files (CMake)

A toolchain file is a CMake snippet applied **before** `project()` logic runs.
Example sketches ship under:

| File | Use |
|------|-----|
| [`cmake/toolchains/aarch64-linux-gnu.cmake.example`](../../cmake/toolchains/aarch64-linux-gnu.cmake.example) | Linux host → aarch64-linux-gnu |
| [`cmake/toolchains/x86_64-linux-gnu.cmake.example`](../../cmake/toolchains/x86_64-linux-gnu.cmake.example) | Linux host → x86_64-linux-gnu |
| [`cmake/toolchains/osx-arch.cmake.example`](../../cmake/toolchains/osx-arch.cmake.example) | macOS `CMAKE_OSX_ARCHITECTURES` helper |

Copy, rename to `.cmake`, and adjust compiler paths for your distro:

```bash
cp cmake/toolchains/aarch64-linux-gnu.cmake.example \
   cmake/toolchains/aarch64-linux-gnu.cmake   # local only; gitignored if you prefer
```

**Never** hardcode secrets in toolchain files; only paths and triples.

---

## 8. Modern diagnostic & optimization tooling (any ISA)

### Sanitizers (debug/CI)

```bash
cmake -S . -B build_asan -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build_asan -j
```

| Tool | Catches |
|------|---------|
| ASan | buffer overflow, use-after-free |
| UBSan | UB (shift, null, alignment) |
| TSan | data races (not always free with lock-free code) |

### Static analysis

- `clang-tidy`, `clang-analyzer`
- Include what you use (IWYU), `cppcheck`

### Binary inspection

```bash
# What ISA is this binary for?
file ./build/lib_smoke
llvm-objdump -d --triple=arm64-apple-darwin ./build/lib_smoke | head
# Linux:
readelf -h ./build/lib_smoke
objdump -d ./build/lib_smoke | head
```

### LTO / PGO (release)

```bash
# Thin LTO (Clang)
cmake -S . -B build_lto -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON

# PGO is multi-step: instrument → run workload → recompile with profile
# See Clang/GCC PGO docs; worth it for stable hot paths (queues, codecs).
```

### Benchmarking on the right CPU

- Measure **on the target architecture** (or a matching cloud instance).
- Apple Silicon numbers do not predict Intel server latency.
- This repo: `make bench` / `bench_queues` — run natively on each platform you ship.

---

## 9. How this repository fits

| Concern | What we do |
|---------|------------|
| Dialect | C++26, extensions off |
| Generator | Default Make or **Ninja** (`make ninja`, presets) |
| Packages | Host-native Homebrew / apt; FetchContent for some deps |
| ARM / Intel | Same sources; no separate trees |
| Cross | Documented here; full mesh expects **native** deps per arch |
| SBE / ll modules | Header-heavy; compile flags apply uniformly |
| HFT-adjacent | See blueprint: prefer portable `-march` + measure tails on target |

### Recommended matrix for CI

| Job | Runner | Generator | Notes |
|-----|--------|-----------|--------|
| macOS arm64 | `macos-14` | Ninja | Primary green bar today |
| Linux x86_64 | `ubuntu-24.04` | Ninja | Full or roadmap smoke |
| Linux aarch64 | `ubuntu-24.04-arm` (when available) | Ninja | Native aarch64 |
| Cross | Optional nightly | Toolchain file | Only if sysroot is maintained |

---

## 10. Practical recipes for *this* project

### Native Release (ARM Mac)

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix)"
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Or: `make ninja` (see [ninja-build.md](ninja-build.md)).

### Native Release (Linux x86_64)

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS_RELEASE="-O3 -march=x86-64-v2"
cmake --build build -j$(nproc)
```

### Debug + sanitizers (any host)

```bash
cmake -S . -B build_san -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build_san -j
```

### Cross sketch (only after deps exist for target)

```bash
cmake -S . -B build_cross -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$PWD/cmake/toolchains/aarch64-linux-gnu.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/opt/aarch64-sysroot/usr
cmake --build build_cross -j
```

---

## 11. Checklist

- [ ] Know host triple (`c++ -dumpmachine`)  
- [ ] One build directory **per** compiler × arch × build type  
- [ ] Prefer **Ninja** for large graphs  
- [ ] Ship portable `-march` / OSX arch; reserve `native` for local benches  
- [ ] Measure latency **on the target ISA**  
- [ ] Cross only with a real **sysroot** or **container** for dependencies  
- [ ] Keep hot-path code small (i-cache) — flags do not fix bloated call graphs  

---

## 12. Further reading

- CMake: [Toolchains](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html)  
- Clang: [Cross-compilation](https://clang.llvm.org/docs/CrossCompilation.html)  
- GCC: [Submodel options](https://gcc.gnu.org/onlinedocs/gcc/Submodel-Options.html) (ARM / x86)  
- x86-64 psABI levels (v2/v3/v4)  
- Apple: [Building a Universal macOS Binary](https://developer.apple.com/documentation/apple-silicon/building-a-universal-macos-binary)
