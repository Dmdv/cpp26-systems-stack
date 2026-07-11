# Layer 1 — Hardware & OS Tuning

Low-latency C++ is **hardware-sympathetic**. Jitter from the OS often dominates algorithmic improvements.

## Checklist

| Topic | Why it matters | In this repo |
|-------|----------------|--------------|
| CPU isolation | Stops scheduler from migrating/preempting hot threads | Documented boot args |
| Thread pinning | Cache warmth + stable TSC domain | `ll::pin_current_thread_to_cpu` (Linux) |
| NUMA | Cross-socket memory is a silent multi-µs tax | `numactl` notes |
| Power states | C-state wakeups add µs–ms | BIOS + governor notes |
| Hugepages | Fewer TLB misses on large rings/arenas | Operator notes |
| **hwloc** | Portable topology (cores, caches, NUMA) | **OPTIONAL** — `[industry][hwloc]` |
| **libnuma** | Bind allocations to local socket | Linux soft-detect / **DOC** |

Industry install: `brew install hwloc` · full map: [07-industry-libraries.md](07-industry-libraries.md).

## Linux isolation (lab machines)

```text
# /etc/default/grub  (example — validate for your distro)
GRUB_CMDLINE_LINUX_DEFAULT="... isolcpus=2,3 nohz_full=2,3 rcu_nocbs=2,3"
```

```bash
# Run app memory-local to node 0, CPUs reserved
numactl --cpunodebind=0 --membind=0 ./systems_app
```

```cpp
#include "ll/affinity.hpp"
ll::pin_current_thread_to_cpu(2);  // Linux only
```

## macOS reality

Apple platforms do **not** expose Linux-style `isolcpus`. This stack uses:

```cpp
ll::set_current_thread_interactive_qos();  // QOS_CLASS_USER_INTERACTIVE
```

Expect **more tail noise** on laptops than on isolated Linux servers — design measurements accordingly.

## Power & frequency

- BIOS: disable deep C-states for latency labs when possible.  
- OS: `performance` governor (`cpupower frequency-set -g performance`).  
- Turbo can help mean throughput but **increase jitter** — know your goal.

## Hugepages

```bash
echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
```

Use hugepage-backed arenas for multi-GB order books or capture buffers (future `ll` backend).

## Module API

```cpp
#include "ll/affinity.hpp"

const char* backend = ll::affinity_backend();
bool pinned = ll::pin_current_thread_to_cpu(/*cpu=*/2);
bool qos = ll::set_current_thread_interactive_qos();
// ll::kLinuxIsolationNotes — printable operator cheat sheet
```

## Tests

- `tests/test_ll_modules.cpp` tag `[ll][affinity]`
