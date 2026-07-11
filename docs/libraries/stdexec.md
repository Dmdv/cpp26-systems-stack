# stdexec (P2300 senders / receivers)

**What it is:** NVIDIA’s reference implementation of **C++ async with senders & receivers** (wg21 P2300).  
**Install:** **Not Homebrew** — this project pulls it via CMake `FetchContent` (tag `nvhpc-26.05`).  
**Smoke:** `tests/test_libs.cpp` tag `[stdexec]`, main smoke “stdexec”

## Why stdexec?

| Strength | Meaning |
|----------|---------|
| **Composable async** | Pipe work: `schedule | then | then | …` |
| **Standard direction** | Aligns with C++26-era execution model |
| **Lazy by default** | Building a sender does not run work until connected/started |
| **Scheduler abstraction** | Same algorithm, different execution contexts |

**Not a replacement for:** Asio networking, Taskflow DAGs, TBB data-parallel loops — it **composes async work** across contexts.

## Mental model

```text
  Sender     =  "description of async work" (lazy)
  Receiver   =  completion channel (set_value / set_error / set_stopped)
  Scheduler  =  "where to run" (thread pool, UI thread, …)
  Operation  =  connected sender+receiver, then start()

  schedule(sched) | then(f) | then(g)   →   still lazy
  sync_wait(sender)                     →   run + block for result
```

```text
  [scheduler] --schedule--> [then f] --then--> [then g] --sync_wait--> value
```

Contrast:

| Style | Eager? | Example |
|-------|--------|---------|
| Folly `Future` | Often starts when created | `makeFuture(1).thenValue(...)` |
| stdexec sender | **Lazy** until started | `schedule \| then` then `sync_wait` |
| Asio handler | Callback on completion | `async_read(..., handler)` |

## Headers (this project)

```cpp
#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>   // NVIDIA exec:: helpers
```

Namespace: `stdexec::` for core; `exec::` for pool utilities.

## Minimal examples

### 1. Schedule + then + sync_wait

```cpp
#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>
#include <atomic>

int main() {
  exec::static_thread_pool pool{2};
  auto sched = pool.get_scheduler();

  std::atomic<int> n{0};

  auto work = stdexec::schedule(sched)
            | stdexec::then([&] { n.store(99); });

  auto result = stdexec::sync_wait(std::move(work));
  // result.has_value() == true
  // n == 99
  // void senders: sync_wait → optional of empty tuple
}
```

Same idea as `tests/test_libs.cpp` and `src/main.cpp`.

### 2. Transform a value through the pipe

```cpp
#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>

int main() {
  exec::static_thread_pool pool{2};
  auto sched = pool.get_scheduler();

  auto snd = stdexec::schedule(sched)
           | stdexec::then([] { return 21; })
           | stdexec::then([](int v) { return v * 2; });

  auto [answer] = stdexec::sync_wait(std::move(snd)).value();
  // answer == 42
}
```

### 3. Conceptual: error and stop (API shape)

Senders complete in one of three ways:

| Channel | Meaning |
|---------|---------|
| `set_value` | Success payload |
| `set_error` | Error (exception_ptr or custom) |
| `set_stopped` | Cancellation |

Algorithms like `upon_error`, `let_value`, `when_all` build on that (see docs; evolve with stdexec version).

### 4. Where it sits next to Asio

Typical bridge (conceptual):

```text
Asio completes socket read on io_context
  → package buffer
  → stdexec::then on a CPU scheduler to parse (simdjson)
  → another then to publish
```

You can also stay pure Asio callbacks; stdexec shines when you want **portable async algorithms** not tied to one I/O library.

## stdexec vs Folly Future vs Asio vs Taskflow

| Need | Prefer |
|------|--------|
| Lazy composable CPU async | **stdexec** |
| Service utilities + Future ecosystem | **Folly** |
| Sockets, timers, Beast | **Asio** |
| Explicit multi-stage graph | **Taskflow** |
| Parallel reduce over arrays | **TBB** |

## Project integration

CMake (`CMakeLists.txt`):

```cmake
FetchContent_Declare(
  stdexec
  GIT_REPOSITORY https://github.com/NVIDIA/stdexec.git
  GIT_TAG        nvhpc-26.05
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(stdexec)
target_link_libraries(... STDEXEC::stdexec)
```

No extra install step — first configure clones stdexec.

## Commands

```bash
make build
cd build && ctest -R stdexec --output-on-failure
./build/lib_smoke   # prints stdexec schedule|then|sync_wait
```

## Pitfalls

1. **Forgetting `sync_wait` / `start`** — building a sender does nothing.
2. **`sync_wait` on void** — returns `optional<tuple<>>`, not a value; don’t structured-bind a single name unless the sender yields one.
3. **Blocking the wrong thread** — `sync_wait` blocks the caller; don’t call it on Asio’s only io thread.
4. **API flux** — stdexec tracks proposals; pin a `GIT_TAG` (this project does).
5. **Not for data-parallel map-reduce** — use TBB; use stdexec to *orchestrate* async steps around that work.

## Further reading

- [NVIDIA/stdexec](https://github.com/NVIDIA/stdexec)
- P2300 / `std::execution` papers (wg21)
- Project: `tests/test_libs.cpp` (`stdexec static_thread_pool`), `src/main.cpp` (`smoke_stdexec`)
