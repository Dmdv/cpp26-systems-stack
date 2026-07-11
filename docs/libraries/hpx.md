# HPX (High Performance ParalleX)

**What it is:** C++ runtime for fine-grained parallelism (and optionally distributed computing).  
**Install:** **Not on Homebrew** — local prefix via `make install-hpx` → `~/cpp-deps/hpx`  
**Enable:** `make hpx` or `-DLIB_SMOKE_WITH_HPX=ON`  
**Smoke:** `tests/test_hpx.cpp`, main smoke section “HPX”

## Why use HPX?

| Use HPX when… | Prefer something else when… |
|---------------|-----------------------------|
| You want **parallel algorithms** with a rich runtime | Simple loop parallel → **TBB** |
| You need **millions of lightweight tasks** | Explicit DAG of a few stages → **Taskflow** |
| You may scale to **multi-node** later | Pure networking → **Asio** |
| You like **future-based** composition | Standard-bound async → **stdexec** |

HPX is a **runtime**, not a utility grab-bag (that’s Folly).

## Why not Homebrew?

There is no `hpx` formula in homebrew-core. HPX is large, version-sensitive (Asio APIs), and often built with custom options (networking on/off, allocators, MPI).

This project’s install script builds **local-only** HPX:

- `HPX_WITH_NETWORKING=OFF`
- `HPX_WITH_DISTRIBUTED_RUNTIME=OFF`
- Fetched Asio (avoids clash with Homebrew Asio 1.36)
- Install prefix: `~/cpp-deps/hpx`

```bash
make install-hpx     # once (several minutes)
make hpx             # configure + build + test + smoke
# full stack:
make full
```

## Minimal examples

HPX programs typically use `hpx/hpx_main.hpp` so `main` runs inside the runtime
(this project’s tests do that).

### 1. Ready future

```cpp
#include <hpx/hpx_main.hpp>
#include <hpx/future.hpp>
#include <iostream>

int main() {
  auto f = hpx::make_ready_future(7);
  std::cout << f.get() << "\n";
  return 0;
}
```

### 2. Continuation

```cpp
#include <hpx/hpx_main.hpp>
#include <hpx/future.hpp>

int main() {
  auto f = hpx::make_ready_future(3).then([](hpx::future<int> r) {
    return r.get() + 4;
  });
  return f.get() == 7 ? 0 : 1;
}
```

### 3. Parallel reduce

```cpp
#include <hpx/hpx_main.hpp>
#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <numeric>
#include <vector>

int main() {
  std::vector<int> v(1000);
  std::iota(v.begin(), v.end(), 1);

  auto sum = hpx::reduce(hpx::execution::par, v.begin(), v.end(), 0);
  // sum == 1000*1001/2
  return 0;
}
```

### 4. Parallel `for_each`

```cpp
#include <hpx/hpx_main.hpp>
#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <vector>

int main() {
  std::vector<int> v(64, 1);
  hpx::for_each(hpx::execution::par, v.begin(), v.end(),
                [](int& x) { x *= 2; });
  return 0;
}
```

## Execution policies (local)

| Policy | Meaning |
|--------|---------|
| `hpx::execution::seq` | Sequential |
| `hpx::execution::par` | Parallel |
| `hpx::execution::par_unseq` | Parallel + vectorisation-friendly (where supported) |

## How HPX relates to your stack

```text
Asio          →  I/O, sockets, timers
Taskflow      →  Explicit multi-stage DAGs (pipelines)
TBB           →  Shared-memory parallel algorithms (lighter dependency)
HPX           →  Parallel runtime (+ future distributed story)
stdexec       →  Standard-oriented async composition
Folly Future  →  Service-oriented async utilities
```

**Rule of thumb:**  
- Feed ingestion / HTTP → **Asio/Beast**  
- Pipeline stages → **Taskflow**  
- Data-parallel hot loops → **TBB** or **HPX**  
- Don’t run heavy HPX work *on* Asio io_context threads

## CMake integration

```cmake
find_package(HPX CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE HPX::hpx)
hpx_setup_target(my_app)   # entry-point / wrap as needed
```

This project:

- `LIB_SMOKE_WITH_HPX`
- `LIB_SMOKE_HPX_ROOT` (default `$HOME/cpp-deps/hpx`)
- Targets: `lib_smoke`, `test_hpx` call `hpx_setup_target`

## Project commands

```bash
make deps-check      # shows whether HPX is installed
make install-hpx
make hpx
./build_hpx/test_hpx
cd build_hpx && ctest -R hpx --output-on-failure
```

## Common pitfalls

1. **Building HPX 1.11.0 against Homebrew Asio 1.36** — resolver API break; use master + fetch Asio (script does this).
2. **Forgetting `hpx_setup_target` / `hpx_main`** — runtime may not start correctly.
3. **Expecting networking** — this install is local-only by design.
4. **Mixing HPX’s installed Asio headers** with standalone Homebrew Asio carelessly — CMake prefers Homebrew for smoke Asio includes.

## Further reading

- [HPX docs](https://hpx-docs.stellar-group.org/)
- [STEllAR-GROUP/hpx](https://github.com/STEllAR-GROUP/hpx)
- Project: `tests/test_hpx.cpp`, `scripts/install_hpx.sh`
