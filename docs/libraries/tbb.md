# oneTBB (Threading Building Blocks)

**What it is:** Intel’s shared-memory parallel library — algorithms, task arena, concurrent containers.  
**Install:** Homebrew `tbb` (you have 2022/2023-line).  
**Smoke:** `tests/test_libs.cpp` tag `[tbb]`, main smoke “oneTBB”

## Why TBB?

| Strength | Example |
|----------|---------|
| **Data-parallel algorithms** | `parallel_for`, `parallel_reduce`, `parallel_sort` |
| **Work-stealing** | Load balance irregular work |
| **Concurrent containers** | `concurrent_queue`, `concurrent_hash_map` |
| **Mature / boring** | Production default for “make this loop multi-core” |

**Not for:** explicit multi-stage DAGs (Taskflow), async sockets (Asio), distributed nodes (HPX).

## Mental model

```text
  blocked_range   →  split the index space into grains
  parallel_*      →  run body on grains across threads
  task_arena      →  optional isolation of thread pool size
```

TBB owns a **global scheduler**. You describe *what* to parallelize; it schedules *how*.

## Minimal examples

### 1. `parallel_reduce` (sum)

```cpp
#include <tbb/parallel_reduce.h>
#include <tbb/blocked_range.h>
#include <iostream>

int main() {
  const int n = 5000;

  const int sum = tbb::parallel_reduce(
      tbb::blocked_range<int>(1, n + 1),
      0,  // identity
      [](const tbb::blocked_range<int>& r, int local) {
        for (int i = r.begin(); i != r.end(); ++i) local += i;
        return local;
      },
      std::plus<>{});

  std::cout << sum << "\n";  // n*(n+1)/2
}
```

Same pattern as `tests/test_libs.cpp` and `src/main.cpp`.

### 2. `parallel_for` (transform in place)

```cpp
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <vector>

void double_all(std::vector<double>& v) {
  tbb::parallel_for(tbb::blocked_range<size_t>(0, v.size()),
                    [&](const tbb::blocked_range<size_t>& r) {
                      for (size_t i = r.begin(); i != r.end(); ++i)
                        v[i] *= 2.0;
                    });
}
```

### 3. Simple form with integers (grain size optional)

```cpp
#include <tbb/parallel_for.h>
#include <vector>

void fill_iota(std::vector<int>& v) {
  tbb::parallel_for(size_t{0}, v.size(), [&](size_t i) {
    v[i] = static_cast<int>(i);
  });
}
```

### 4. Concurrent queue (producer / consumer sketch)

```cpp
#include <tbb/concurrent_queue.h>
#include <tbb/parallel_for.h>
#include <atomic>

void sketch() {
  tbb::concurrent_queue<int> q;
  std::atomic<int> consumed{0};

  // producers
  tbb::parallel_for(0, 100, [&](int i) { q.push(i); });

  // single-thread drain (or parallel consumers carefully)
  int x;
  while (q.try_pop(x)) consumed.fetch_add(1);
}
```

### 5. Limit threads with `task_arena`

```cpp
#include <tbb/task_arena.h>
#include <tbb/parallel_for.h>

void on_four_cores() {
  tbb::task_arena arena(4);
  arena.execute([&] {
    tbb::parallel_for(0, 1000, [](int) { /* work */ });
  });
}
```

Useful when you co-exist with Asio thread pools and don’t want TBB to grab all cores.

## Algorithms you should know

| API | Role |
|-----|------|
| `parallel_for` | Independent iterations |
| `parallel_reduce` | Combine to one value |
| `parallel_invoke` | Run 2–N callables in parallel |
| `parallel_sort` | Parallel sort |
| `parallel_pipeline` | Token pipeline (heavier than Taskflow DAGs) |
| `enumerable_thread_specific` | Per-thread accumulators |

## TBB vs Taskflow vs HPX

| Scenario | Pick |
|----------|------|
| “Sum this array / map this range” | **TBB** |
| “Stage A then B then C, with branches” | **Taskflow** |
| Need HPX ecosystem / distributed later | **HPX** |
| Network reactor | **Asio** (offload CPU to TBB) |

**Composition pattern for feeds:**

```text
Asio receives packet
   → push to SPSC/MPSC queue (or concurrent_queue)
   → Taskflow stage or TBB parallel_for over batch
   → publish result
```

## Pitfalls

1. **Nested parallelism** — OK with TBB, but watch oversubscription with other pools.
2. **False sharing** — per-iteration writes to adjacent cache lines; use local accumulators + reduce.
3. **Exceptions** — TBB propagates; know your cancellation story.
4. **Linking** — need `TBB::tbb` (this project does); not header-only.
5. **Don’t call `parallel_for` from latency-critical single-item path** — overhead only pays off at batch scale.

## Project commands

```bash
make build
./build/lib_smoke                 # includes TBB reduce check
cd build && ctest -R tbb --output-on-failure
```

CMake: `find_package(TBB CONFIG REQUIRED)` → `TBB::tbb`.

## Further reading

- [oneTBB documentation](https://oneapi-src.github.io/oneTBB/)
- Project: `tests/test_libs.cpp` (`oneTBB parallel_reduce`), `src/main.cpp` (`smoke_tbb`)
