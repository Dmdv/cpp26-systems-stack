# CLion setup — full IntelliSense, types, and hints

This project is **CMake-first**. CLion should load **CMake Presets** so every
target, include path, and generated header participates in the code model.

## 1. Open the project

1. **File → Open** → select the repository root (folder with `CMakeLists.txt`).
2. When prompted, choose **Open as CMake project** (not “Open as Directory” only).
3. In the CMake tool window / **Settings → Build, Execution, Deployment → CMake**:
   - Enable **CMake presets**
   - Profile: **`clion-debug`** (recommended)
   - Alternative: `clion-release`, `clion-relwithdebinfo`

Presets live in [`CMakePresets.json`](../CMakePresets.json).

## 2. Local paths (Homebrew / HPX)

Copy and edit if your prefixes differ:

```bash
cp CMakeUserPresets.json.example CMakeUserPresets.json
# edit CMAKE_PREFIX_PATH / HOMEBREW_PREFIX
```

`CMakeUserPresets.json` is gitignored.

Default macOS Apple Silicon prefix: `/opt/homebrew`.

```bash
make install-industry   # hwloc flatbuffers google-benchmark mimalloc
# base deps: see README
```

## 3. Build the IDE index target (important)

CLion indexes symbols from **translation units in CMake targets**. Header-only
`ll::*` APIs are pulled into a dedicated target so types are always visible:

| Target | Purpose |
|--------|---------|
| **`ide_index`** | Includes all `ll/*`, SBE, Asio, Beast, TBB, Taskflow, simdjson, … |
| `lib_smoke` | Full mesh runtime smoke |
| `test_*` | Tests (also excellent for navigation) |

```bash
cmake --preset clion-debug
cmake --build --preset ide-index
# or in CLion: select target ide_index → Build
```

After a successful **Reload CMake Project**, you should get:

- Completion on `ll::SpscQueue`, `ll::HdrHistogramC`, `market::sbe::Tick`, …
- Go-to-definition into `include/ll/*.hpp` and `generated/sbe/market_sbe/`
- Macro-aware branches (`LL_HAS_*`) matching your configure

## 4. compile_commands.json

Presets set `CMAKE_EXPORT_COMPILE_COMMANDS=ON`. After configure:

```text
build/clion-debug/compile_commands.json
```

Optional convenience (also used by Makefile):

```bash
make clion   # configure clion-debug + link compile_commands.json at repo root
```

[`.clangd`](../.clangd) points at `build/clion-debug` for clangd-based tooling.

## 5. Shared run configurations

If present under `.idea/runConfigurations/`, CLion offers ready-made runs for:

- `lib_smoke`
- `ide_index`
- `test_roadmap_stack`
- `example_sbe_codegen`

Otherwise: **Run → Edit Configurations → + → CMake Application** and pick the target.

## 6. Tips when types look “unknown”

| Symptom | Fix |
|---------|-----|
| Red code in headers only | Build **`ide_index`** or open a `.cpp` that includes them |
| Missing Homebrew packages | `make deps-check` / `make install-industry` |
| Wrong C++ standard | Preset forces **C++26**; check CMake output |
| SBE types missing | Ensure `generated/sbe/market_sbe/*.h` exists; `STACK_WITH_SBE_CODEGEN=ON` |
| Stale index | **Tools → CMake → Reset Cache and Reload Project** |
| Folly/HPX types missing | Configure with `LIB_SMOKE_WITH_FOLLY=ON` / HPX (optional profiles) |

## 7. What CLion should list (Project tool window)

Under **Targets** after reload:

- `ide_index`, `lib_smoke`, `smoke_deps`, `ll_headers`, `stack_industry`
- All `test_*`, `example_*`, optional `bench_queues`
- Headers under `include/ll/` and `generated/sbe/` (via `FILE_SET HEADERS`)

## 8. Makefile helpers

```bash
make clion          # cmake --preset clion-debug + compile_commands link
make clion-index    # build ide_index
make clion-test     # ctest on clion-debug build tree
```
