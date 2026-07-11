# Tutorial: Building this project with Ninja

**Ninja is not a compiler.** It is a small, fast **build system** (like Make).
Your **compiler** is still Clang or GCC; CMake generates a `build.ninja` file,
and Ninja runs the compile/link commands as efficiently as possible.

```text
  source  ──►  CMake (-G Ninja)  ──►  build.ninja  ──►  Ninja  ──►  binaries
                  │                       │                │
                  │                       │                └─ invokes c++ / clang++
                  └─ finds packages, sets flags, writes compile_commands.json
```

This repository stays **generator-agnostic** in `CMakeLists.txt`. You choose Ninja
at **configure** time (CLI, Makefile helpers, or CMake Presets). Nothing in the
project *requires* Ninja — it is the recommended high-performance generator.

---

## 1. Why use Ninja?

| Topic | Make (default on many systems) | Ninja |
|-------|--------------------------------|-------|
| Startup | Parses large Makefiles | Tiny, constant-time graph load |
| Parallelism | Good with `-j` | Excellent; designed for huge graphs |
| Rebuild | Fine | Often faster for incremental C++ |
| IDE | Works | CLion / VS Code often prefer Ninja |
| Windows | MinGW Make quirks | Strong with MSVC + Ninja |

For this stack (many targets: `lib_smoke`, tests, examples, FetchContent deps),
Ninja tends to give **snappier incremental builds** after the first configure.

---

## 2. Install Ninja + a C++26 compiler

### macOS (Homebrew)

```bash
brew install ninja cmake
# Compiler: Apple Clang from Xcode CLT, or llvm
xcode-select --install   # if needed
clang++ --version
ninja --version
```

### Linux (Debian/Ubuntu)

```bash
sudo apt-get install -y ninja-build cmake g++ 
# or: clang++-18 / g++-14 with C++26 support
ninja --version
```

### Verify

```bash
which ninja
cmake --help | grep -i ninja    # should list "Ninja" generators
```

You still need the project’s library dependencies (see root [README](../../README.md)):

```bash
# macOS example
brew install fmt spdlog tbb asio boost taskflow \
  nlohmann-json simdjson eigen protobuf grpc \
  range-v3 abseil catch2 googletest
make install-industry   # optional industry extras
```

---

## 3. Configure with the Ninja generator

From the **repository root**:

### Option A — one-shot CMake (explicit)

```bash
cmake -S . -B build_ninja -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix)" \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DLIB_SMOKE_WITH_FOLLY=OFF \
  -DLIB_SMOKE_WITH_HPX=OFF
```

What this does:

| Flag | Meaning |
|------|---------|
| `-S .` | Source tree (this repo) |
| `-B build_ninja` | Out-of-source build directory (keeps tree clean) |
| **`-G Ninja`** | Generate `build.ninja` instead of Unix Makefiles |
| `CMAKE_BUILD_TYPE` | `Debug` / `Release` / `RelWithDebInfo` |
| `CMAKE_EXPORT_COMPILE_COMMANDS` | Emit `compile_commands.json` for clangd / CLion |

After success you should see:

```text
build_ninja/build.ninja
build_ninja/compile_commands.json
build_ninja/CMakeCache.txt
```

Confirm the generator:

```bash
grep CMAKE_GENERATOR: build_ninja/CMakeCache.txt
# CMAKE_GENERATOR:INTERNAL=Ninja
```

### Option B — Makefile helpers (this repo)

Optional targets wrap the same CMake flags (isolated tree `build_ninja/`):

```bash
make ninja-help          # list Ninja-related targets
make configure-ninja     # cmake -G Ninja → build_ninja/
make build-ninja         # cmake --build build_ninja
make test-ninja          # ctest in build_ninja/
make run-ninja           # ./build_ninja/lib_smoke
make ninja               # configure + build + test (all-in-one)
make examples-ninja      # build & run example binaries via Ninja tree
make compile-commands-ninja  # symlink compile_commands.json → build_ninja/
```

These **do not replace** the default `make` / `build/` path; they sit alongside it.

### Option C — CMake Presets (already Ninja)

[`CMakePresets.json`](../../CMakePresets.json) uses **`"generator": "Ninja"`** for
`clion-debug`, `default`, etc.:

```bash
cmake --preset clion-debug
cmake --build --preset clion-debug
ctest --preset clion-debug
```

Binary dir: `build/clion-debug/` (see preset `binaryDir`).

### Option D — CLion

1. Open the repo as a CMake project.  
2. Enable preset **`clion-debug`** (generator Ninja).  
3. Build target `ide_index` for IntelliSense — [docs/clion.md](../clion.md).

---

## 4. Build with Ninja

Once configured, prefer **`cmake --build`** (generator-agnostic) or call `ninja` in the build dir.

### Recommended (portable)

```bash
cmake --build build_ninja -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu)"
# or
cmake --build build_ninja --parallel
```

### Direct Ninja

```bash
cd build_ninja
ninja                  # default target `all` (note: ide_index is EXCLUDE_FROM_ALL)
ninja -j8              # limit parallelism
ninja lib_smoke        # single target
ninja test_roadmap_stack example_sbe_codegen
ninja -t targets       # list targets
ninja -t query lib_smoke
```

### Useful Ninja commands

| Command | Purpose |
|---------|---------|
| `ninja -C build_ninja` | Build without `cd` |
| `ninja -t clean` | Remove built artifacts (keep CMake cache) |
| `ninja -d explain` | Why a target rebuilt |
| `ninja -n` | Dry-run |
| `ninja -v` | Verbose compile lines |

---

## 5. Test and run

```bash
# From build tree
cd build_ninja && ctest --output-on-failure -j8

# Or via cmake
ctest --test-dir build_ninja --output-on-failure

# Integration binary
./build_ninja/lib_smoke

# Roadmap / industry filters
ctest --test-dir build_ninja -R 'll|industry|roadmap' --output-on-failure
```

Makefile equivalents:

```bash
make test-ninja
make run-ninja
```

---

## 6. Incremental workflow (day to day)

```bash
# First time
make ninja                 # or: configure-ninja && build-ninja && test-ninja

# Edit a header / cpp
cmake --build build_ninja -j    # Ninja rebuilds only what changed

# Change CMake options
cmake -S . -B build_ninja -G Ninja -DSTACK_WITH_BENCHMARK=OFF
cmake --build build_ninja -j
```

**Do not** mix generators in one build directory. If you previously configured
`build/` with Unix Makefiles, either:

```bash
rm -rf build_ninja   # clean Ninja tree only
# or
cmake -S . -B build_ninja -G Ninja   # separate tree (preferred)
```

Re-running `cmake -G Ninja` **inside** a Makefile-generated tree will error;
use a dedicated folder (`build_ninja/`).

---

## 7. Compiler selection (still not Ninja)

Ninja only **orchestrates** the compiler CMake selected:

```bash
# Force Clang
cmake -S . -B build_ninja -G Ninja \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix)"

# Force GCC (Linux)
cmake -S . -B build_ninja -G Ninja \
  -DCMAKE_CXX_COMPILER=g++-14 \
  -DCMAKE_C_COMPILER=gcc-14
```

Or via environment **before** first configure:

```bash
export CXX=clang++ CC=clang
cmake -S . -B build_ninja -G Ninja ...
```

Check:

```bash
grep CMAKE_CXX_COMPILER: build_ninja/CMakeCache.txt
```

---

## 8. compile_commands.json for IDEs

With `CMAKE_EXPORT_COMPILE_COMMANDS=ON` (default in our Makefile / presets):

```bash
ls build_ninja/compile_commands.json
ln -sfn build_ninja/compile_commands.json compile_commands.json   # optional for clangd
# or
make compile-commands-ninja
```

CLion with the **clion-debug** preset already uses Ninja + compile commands under
`build/clion-debug/`.

---

## 9. Optional profiles (Folly / HPX) with Ninja

Same options as the main README, only generator changes:

```bash
cmake -S . -B build_ninja_full -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix);$HOME/cpp-deps/hpx" \
  -DLIB_SMOKE_WITH_FOLLY=ON \
  -DLIB_SMOKE_WITH_HPX=ON \
  -DLIB_SMOKE_HPX_ROOT=$HOME/cpp-deps/hpx

cmake --build build_ninja_full -j
ctest --test-dir build_ninja_full --output-on-failure
```

---

## 10. Troubleshooting

| Symptom | Cause / fix |
|---------|-------------|
| `CMake Error: Could not create named generator Ninja` | Install `ninja` / `ninja-build`; ensure it is on `PATH` |
| `Generator: … does not match … Ninja` | Build dir was configured with another generator → use a new `-B` dir or delete the tree |
| Slow first build | Normal: FetchContent (`stdexec`, HdrHistogram_c, …) + many TUs |
| `ide_index` missing from `ninja all` | Intentional `EXCLUDE_FROM_ALL` → `ninja ide_index` or CLion target |
| Link errors for Homebrew libs | Pass `-DCMAKE_PREFIX_PATH=$(brew --prefix)` |
| C++26 not supported | Upgrade Clang/GCC; project sets `CMAKE_CXX_STANDARD 26` |

---

## 11. Mental model (one page)

1. **Compiler** = Clang/GCC (`CMAKE_CXX_COMPILER`).  
2. **Build system** = Ninja (`-G Ninja`).  
3. **Meta-build** = CMake (`CMakeLists.txt`).  
4. **Packages** = Homebrew / apt / FetchContent.  
5. **Driver scripts** = `make ninja` or `cmake --preset …` (optional sugar).

Minimal happy path:

```bash
brew install ninja cmake   # once
make ninja                 # configure-ninja + build-ninja + test-ninja
./build_ninja/lib_smoke
```

---

## Related docs

| Doc | Topic |
|-----|--------|
| [README.md](../../README.md) | Dependencies, full feature map |
| [docs/clion.md](../clion.md) | CLion + Ninja presets |
| [CMakePresets.json](../../CMakePresets.json) | Preset `generator: Ninja` |
| [industry-stack.md](industry-stack.md) | Optional industry libraries |
| [modern-cpp-tooling-arm-intel.md](modern-cpp-tooling-arm-intel.md) | Compilers, ARM/Intel flags, cross-compilation |
