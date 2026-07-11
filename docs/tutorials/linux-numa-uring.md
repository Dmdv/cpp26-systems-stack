# Tutorial: Linux NUMA + io_uring

Cross-socket memory and classic `read()`/`epoll` syscall paths are common sources
of **tail latency**. This stack provides portable stubs plus real backends when
`libnuma` / `liburing` are present on Linux.

| Header | Role |
|--------|------|
| `ll/linux_numa.hpp` | node discovery, bind, `alloc_onnode` |
| `ll/linux_uring.hpp` | ring open, NOP, file read via SQE/CQE |
| Tests | `[roadmap][numa]`, `[roadmap][uring]` |
| Example | `example_numa_uring` |
| CI | `.github/workflows/ci.yml` (Linux job installs dev packages) |

## Install (Linux)

```bash
# Debian/Ubuntu
sudo apt-get install -y libnuma-dev liburing-dev

# Fedora/RHEL
sudo dnf install -y numactl-devel liburing-devel
```

CMake auto-enables:

- `STACK_WITH_LIBNUMA=ON` → `LL_HAS_LIBNUMA=1` when found  
- `STACK_WITH_LIBURING=ON` → `LL_HAS_LIBURING=1` when found  

On macOS/Windows, the same APIs compile as **stubs** (`available() == false`) so
the portable suite stays green.

## NUMA patterns

```cpp
#include "ll/linux_numa.hpp"

if (ll::numa::available()) {
  int node = ll::numa::preferred_node();
  ll::numa::bind_thread_to_node(node);          // CPUs + preferred memory
  void* p = ll::numa::alloc_onnode(1 << 20, node);
  // … place SPSC rings / arenas here …
  ll::numa::free_onnode(p, 1 << 20);
}
```

Operator companions (outside the process):

```bash
numactl --hardware
numactl --cpunodebind=0 --membind=0 ./systems_app
# boot: isolcpus=… nohz_full=…  (see docs/blueprint/01-hardware-os.md)
```

## io_uring patterns

```cpp
#include "ll/linux_uring.hpp"

ll::uring::Ring ring;
if (ring.open(32) && ring.submit_nop_and_wait()) {
  // ring path is healthy
}
char buf[4096];
int n = ring.read_file("/path/to/journal", buf);  // async read + wait CQE
```

**Guidance:** use uring for **logging, capture, replay, and bulk disk** off the
absolute tick-to-trade path. For NIC ingress at HFT latencies, prefer poll-mode
bypass (DPDK/Onload) — see kernel-bypass lab.

## Run

```bash
./build/example_numa_uring
ctest --test-dir build -R 'roadmap.*(numa|uring)'
```

## CI expectations

| Platform | NUMA | uring |
|----------|------|-------|
| macOS | stub tests pass | stub tests pass |
| Linux + dev packages | bind/alloc exercised when permitted | NOP + optional `/etc/hosts` read |
| Restricted containers | `available()` may be false even with libs | tests tolerate failure |

## Related

- `docs/blueprint/01-hardware-os.md`  
- `docs/tutorials/kernel-bypass-lab.md`
