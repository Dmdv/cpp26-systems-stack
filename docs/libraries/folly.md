# Folly (Facebook Open-source Library)

**What it is:** Meta’s large C++ utility library — futures, containers, strings, concurrency helpers.  
**Install:** `brew install folly` or `make install-folly`  
**Enable in project:** `make folly` or `-DLIB_SMOKE_WITH_FOLLY=ON`  
**Smoke:** `tests/test_folly.cpp`, main smoke section “Folly”

## Why use Folly?

Use Folly when you want **battle-tested service primitives** that go beyond the STL:

| Area | Examples |
|------|----------|
| Strings | `fbstring`, `StringPiece`, `join` |
| Containers | `F14FastMap` / `F14ValueMap` (fast hash maps) |
| Optional / Expected-ish | `Optional`, `Try`, `Expected` (various) |
| Async | `Future` / `Promise`, executors |
| Concurrency | MPMC queues, atomics helpers, synchronized |

It is **not** a networking stack (use Asio) and **not** a parallel algorithm runtime (use TBB/Taskflow/HPX).

## Enable & run

```bash
make install-folly   # once
make folly           # configure + build + test + run smoke
# or
cmake -S . -B build_folly -DLIB_SMOKE_WITH_FOLLY=ON -DCMAKE_PREFIX_PATH=/opt/homebrew
```

CMake target: `Folly::folly`  
Compile define when on: `LIB_SMOKE_HAS_FOLLY=1`

## macOS gotcha (this project already works around it)

Homebrew’s `Folly::folly_deps` can inject the CommandLineTools **C** SDK include path, which redefines `isnan` / `isfinite` as macros and breaks libc++ / fmt / Beast.

This repo:

1. Strips that path in `CMakeLists.txt` after `find_package(folly)`.
2. Uses `src/folly_compat.hpp` to `#undef` math macros before Folly headers when needed.

If you start a new target with Folly + Beast in one TU, include `folly_compat.hpp` first or keep Folly out of Beast translation units (preferred).

## Minimal examples

### 1. `fbstring`

```cpp
#include <folly/FBString.h>
#include <iostream>

int main() {
  folly::fbstring s = "hello";
  s += "-folly";
  std::cout << s << "\n";
}
```

### 2. `Optional`

```cpp
#include <folly/Optional.h>

folly::Optional<int> parse(bool ok) {
  if (!ok) return folly::none;
  return 42;
}
```

### 3. `F14FastMap` (fast hash map)

```cpp
#include <folly/container/F14Map.h>
#include <string>

void demo() {
  folly::F14FastMap<std::string, int> m;
  m["a"] = 1;
  m.emplace("b", 2);
  // m.at("a") == 1
}
```

### 4. `join`

```cpp
#include <folly/String.h>
#include <string>
#include <vector>

void demo() {
  std::vector<std::string> parts{"x", "y", "z"};
  auto s = folly::join(",", parts);  // "x,y,z"
}
```

### 5. `Future` continuation

```cpp
#include <folly/futures/Future.h>

int demo() {
  auto f = folly::makeFuture(21).thenValue([](int v) { return v * 2; });
  return std::move(f).get();  // 42
}
```

## What to learn next (Folly is huge)

Learn **by subset**, not cover-to-cover:

1. **Strings:** `fbstring`, `StringPiece`, `split` / `join`
2. **F14 maps/sets** — often faster than `std::unordered_map` for service caches
3. **Futures** — if you adopt Folly async style in a service
4. **`folly::Executor`** — thread pools for Future continuations
5. **`Synchronized<T>`** — simple shared-state wrapper

Skip initially: full IO/async socket stack, experimental modules, everything under `experimental/`.

## How Folly relates to your stack

| Need | Prefer | Folly role |
|------|--------|------------|
| TCP/HTTP | Asio + Beast | Optional helpers only |
| Task graph | Taskflow | Don’t replace with ad-hoc Futures |
| Parallel reduce | TBB / HPX | Folly Futures are async, not data-parallel |
| Format/log | fmt / spdlog | Keep; Folly uses fmt internally |
| Hash map hot path | F14 | Strong Folly use-case |

## Project commands

```bash
make folly
./build_folly/test_folly
cd build_folly && ctest -R folly --output-on-failure
```

## Further reading

- [Folly GitHub](https://github.com/facebook/folly)
- Project: `tests/test_folly.cpp`, `src/folly_compat.hpp`
