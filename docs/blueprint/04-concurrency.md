# Layer 4 — Concurrency & Lock-Free Programming

Mutexes put threads to sleep. Wakeups are **µs-class** events — fatal for tick-to-trade paths.

## Checklist

| Topic | Module / doc |
|-------|----------------|
| SPSC ring | `ll::SpscQueue` |
| Memory orders | This page + `examples/memory_order` |
| Single-thread actor path | Design section |
| Higher-level graphs | Taskflow, TBB, stdexec (library layer) |

## SPSC algorithm

```text
Producer:  if full → fail; slots[tail]=x; release tail++
Consumer:  if empty → fail; x=slots[head]; release head++
```

Capacity is power-of-two → `index & (Cap-1)`.

```cpp
#include "ll/spsc_queue.hpp"
ll::SpscQueue<Event, 4096> q;
q.try_push(e);
auto e2 = q.try_pop();
```

## Memory ordering (minimum correct mental model)

| Order | Use |
|-------|-----|
| `relaxed` | Counters not used for synchronization |
| `release` | Publish data (store that “unlocks” readers) |
| `acquire` | Observe publication (load before reading payload) |
| `seq_cst` | Default; correct but can be slower |

**Pairing:** writer stores payload, then `release` flag/index; reader `acquire` flag/index, then reads payload.

## Actor / single-thread critical path

Often the fastest design is **not** “more threads,” but:

```text
one pinned thread: poll NIC → decode → book → risk → generate order
```

Synchronization cost becomes **zero**. Parallelism moves to **upstream feed demux** or **downstream logging**.

The assembler conflator’s exclusive-shard model is this idea at multi-core scale:  
**each shard is an actor** — no shared book locks.

## Higher-level concurrency (not replacements for SPSC)

| Library | Use |
|---------|-----|
| Taskflow | Explicit multi-stage DAGs off the absolute hot path |
| TBB | Parallel batch analytics |
| stdexec | Composable async scheduling (P2300 direction) |

## Example & tests

- `examples/spsc/main.cpp`  
- `examples/memory_order/main.cpp`  
- Stress: `tests/test_ll_modules.cpp` `[ll][spsc]`
