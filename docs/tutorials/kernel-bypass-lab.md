# Tutorial: Kernel-bypass lab (DPDK / OpenOnload / EF_VI)

Standard sockets traverse the kernel. HFT and ultra-low-latency game servers often
**poll NIC rings in user space**. This repository does **not** vendor DPDK or
Solarflare SDKs (they require specific NICs, hugepages, and IOMMU setup). Instead
it ships:

1. A poll-mode **interface contract** — `ll::bypass::IPollModeRx`
2. A **stub backend** for tests — `ll::bypass::StubPollModeRx`
3. This lab runbook for dedicated hardware
4. An example that feeds **SBE** buffers through the stub RX path

| Artifact | Path |
|----------|------|
| API | `include/ll/kernel_bypass.hpp` |
| Tests | `[roadmap][bypass]` |
| Example | `example_kernel_bypass` |

## Architecture

```text
   NIC ring  ──poll──►  IPollModeRx::poll(PacketView[])
                              │
                              ▼
                     zero-copy parse (SBE / POD)
                              │
                              ▼
                     strategy / conflator (pinned core)
```

## Lab prerequisites (DPDK)

| Item | Notes |
|------|-------|
| NIC | Intel/Mellanox/etc. with DPDK PMD support |
| OS | Linux with hugepages (`1G` preferred) |
| Packages | DPDK dev package or source build |
| Isolation | `isolcpus`, `nohz_full`, IRQ affinity off hot cores |
| Permissions | hugepage + device bind (`dpdk-devbind.py`) |

```bash
# Example hugepage setup (validate for your distro)
echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
# Bind NIC away from kernel driver (destructive — lab only)
# sudo dpdk-devbind.py --bind=vfio-pci 0000:3b:00.0
```

### Implementing DPDK behind `IPollModeRx`

```cpp
class DpdkPollModeRx final : public ll::bypass::IPollModeRx {
 public:
  Backend backend() const noexcept override { return Backend::Dpdk; }
  // open(): rte_eal_init, rte_eth_dev_configure, rte_eth_rx_queue_setup, start
  // poll(): rte_eth_rx_burst → fill PacketView without copying payload
};
```

Set `LL_HAS_DPDK=1` in your private CMake overlay when linking `libdpdk`.

## Lab prerequisites (Solarflare OpenOnload / EF_VI)

| Item | Notes |
|------|-------|
| NIC | Solarflare/Xilinx (now AMD) adapters common in HFT |
| SDK | OpenOnload package + headers (`onload`, `etherfabric`) |
| Mode | Onloaded TCP/UDP sockets **or** raw EF_VI event queues |

```bash
# Typical (vendor docs supersede)
onload --profile=latency ./systems_app
# EF_VI is lower level: ef_driver_open, ef_pd_alloc, ef_vi_receive_init, …
```

Implement `Backend::OpenOnload` or `Backend::EfVi` the same way: **no malloc in
`poll()`**, pin the polling thread, keep the binary small for i-cache.

## What this repo validates without hardware

```bash
./build/example_kernel_bypass
ctest --test-dir build -R 'roadmap.*bypass'
```

The stub injects an SBE-encoded tick and `poll()` returns a zero-copy view.

## Safety / scope

- Do **not** run `dpdk-devbind` on production hosts without a change window.  
- Kernel bypass disables many host firewall/visibility features — isolate lab VLANs.  
- This monorepo’s default CI never requires a physical NIC.

## Related

- SBE wire path: `docs/tutorials/sbe-codegen.md`  
- NUMA pinning for the poll thread: `docs/tutorials/linux-numa-uring.md`  
- Layer 2 blueprint: `docs/blueprint/02-network-ingress.md`
