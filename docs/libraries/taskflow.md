# Taskflow

**What it is:** Header-only C++ library for **explicit task graphs** (DAGs) — pipelines, conditionals, parallelism.  
**Install:** Homebrew `taskflow` (already in your stack).  
**Smoke:** `tests/test_taskflow.cpp` — `make` / `ctest -R taskflow`

## Why Taskflow?

| Strength | Example |
|----------|---------|
| **Visible DAG** | Parse → Conflate → Aggregate → Report |
| **Dependencies** | `A.precede(B)` — B waits for A |
| **Parallel fan-out** | Many independent tasks, one `run()` |
| **Control flow** | Condition tasks (branching) |
| **Lightweight** | Header-only, no runtime daemon |

**Not for:** sockets/timers (Asio), data-parallel “for all elements” as primary API (TBB/HPX), sender/receiver composition (stdexec).

## Mental model

```text
  tf::Taskflow   =  the graph (tasks + edges)
  tf::Executor   =  the workers that run the graph
  executor.run(flow).wait()  =  submit + block completion
```

```text
     [A] ──precede──► [B] ──precede──► [C]
      │
      └── many independent tasks can run in parallel
```

## Minimal examples

### 1. Linear pipeline (A → B → C)

```cpp
#include <taskflow/taskflow.hpp>
#include <vector>
#include <mutex>

int main() {
  tf::Executor executor;
  tf::Taskflow flow;

  std::vector<int> order;
  std::mutex mu;

  auto A = flow.emplace([&] {
    std::lock_guard lock(mu);
    order.push_back(1);
  });
  auto B = flow.emplace([&] {
    std::lock_guard lock(mu);
    order.push_back(2);
  });
  auto C = flow.emplace([&] {
    std::lock_guard lock(mu);
    order.push_back(3);
  });

  A.precede(B);  // B after A
  B.precede(C);  // C after B

  executor.run(flow).wait();
  // order == {1, 2, 3}
}
```

Matches `tests/test_taskflow.cpp` — “linear dependency graph”.

### 2. Fan-out: many independent tasks

```cpp
#include <taskflow/taskflow.hpp>
#include <atomic>

int main() {
  tf::Executor executor;
  tf::Taskflow flow;

  constexpr int N = 64;
  std::atomic<long long> sum{0};

  for (int i = 1; i <= N; ++i) {
    flow.emplace([&, i] {
      sum.fetch_add(i, std::memory_order_relaxed);
    });
  }

  executor.run(flow).wait();
  // sum == N*(N+1)/2
}
```

No edges between tasks → maximum parallel scheduling.

### 3. Condition task (branching)

```cpp
#include <taskflow/taskflow.hpp>
#include <atomic>

int main() {
  tf::Executor executor;
  tf::Taskflow flow;

  std::atomic<int> path{-1};

  auto init  = flow.emplace([] {});
  auto cond  = flow.emplace([] { return 1; });  // 0 → first successor, 1 → second, …
  auto left  = flow.emplace([&] { path.store(0); });
  auto right = flow.emplace([&] { path.store(1); });

  init.precede(cond);
  cond.precede(left, right);  // successors chosen by return index

  executor.run(flow).wait();
  // path == 1
}
```

### 4. Feed-style pipeline sketch (HFT-ish)

```cpp
#include <taskflow/taskflow.hpp>
#include <string>
#include <vector>

struct Tick { double px; double qty; };

int main() {
  tf::Executor executor{4};
  tf::Taskflow flow;

  std::vector<std::string> raw;
  std::vector<Tick> ticks;
  double vwap = 0.0;

  auto receive = flow.emplace([&] {
    raw = {R"({"px":100,"qty":2})", R"({"px":101,"qty":3})"};
  });
  auto parse = flow.emplace([&] {
    // parse raw → ticks (use simdjson in real code)
    ticks = {{100, 2}, {101, 3}};
  });
  auto aggregate = flow.emplace([&] {
    double notional = 0, qty = 0;
    for (auto& t : ticks) { notional += t.px * t.qty; qty += t.qty; }
    vwap = qty > 0 ? notional / qty : 0;
  });
  auto report = flow.emplace([&] {
    // write vwap / metrics
  });

  receive.precede(parse);
  parse.precede(aggregate);
  aggregate.precede(report);

  executor.run(flow).wait();
}
```

## Patterns worth learning next

| Feature | Use |
|---------|-----|
| `precede` / `succeed` | Edges |
| `tf::Taskflow::composed_of` | Subflows / modules |
| `for_each` / `for_each_index` | Graph-local parallel loops *(note: AppleClang + C++26 + lambdas can hit linkage issues — prefer `emplace` loops if you hit that)* |
| `tf::Semaphore` | Limit concurrency (e.g. max 4 parsers) |
| `observer` | Profiling / tracing tasks |
| Dynamic tasking | Create tasks while running |

## Taskflow vs TBB vs HPX vs Asio

| Need | Prefer |
|------|--------|
| Named stages + dependencies | **Taskflow** |
| Parallel sum/sort/map over arrays | **TBB** (or HPX) |
| Full parallel runtime / multi-node | **HPX** |
| Network I/O | **Asio** — then hand work to Taskflow |

**Golden rule:** don’t block Taskflow workers on network `read()`; Asio completes I/O, then `emplace` CPU work (or use a queue between them).

## Project commands

```bash
make configure && make build
./build/test_taskflow
cd build && ctest -R taskflow --output-on-failure
```

## Further reading

- [Taskflow cookbook](https://taskflow.github.io/)
- Project: `tests/test_taskflow.cpp`
