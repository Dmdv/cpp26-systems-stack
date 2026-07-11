# Layer 2 — Kernel Bypass & Network Ingress

The classic socket path (NIC → kernel → socket buffer → `read`) costs **copies and syscalls**.  
Ultra-low-latency shops poll NIC rings from user space.

## Checklist

| Topic | Status here |
|-------|-------------|
| OpenOnload / EF_VI / DPDK | **Documented boundary** — not vendored drivers |
| Poll-mode user space NIC | GAP (specialized hardware repos) |
| Zero-copy mindset | Asio buffer sequences; POD decode |
| Hot-path encoding | Prefer POD / SBE over JSON/Protobuf text |
| Practical ingress libs | **Asio, Beast, gRPC** shipped & tested |
| **FlatBuffers** | Zero-copy tables | **OPTIONAL** (`schemas/tick.fbs`) |
| **SBE-style POD** | Packed wire cast | **SHIPPED** (`ll/sbe_style.hpp`) |
| **liburing** | Modern async I/O | Linux soft-detect |
| **struct_pack** | Compile-time pack | **DOC** (not vendored) |

## What this repository *does* ship

| Component | Role |
|-----------|------|
| **Asio** | Event loop, timers, sockets |
| **Beast** | HTTP / WebSocket on Asio |
| **simdjson** | When JSON cannot be avoided |
| **protobuf + gRPC** | Service edges, not tick-to-trade hot path |
| **ll::sbe::TickMsg** | 16-byte packed demo (Real Logic SBE for prod schemas) |
| **FlatBuffers** | Zero-copy `market.Tick` when flatc is installed |

See `docs/libraries/asio.md`, `beast.md`, `simdjson.md`, `grpc-protobuf.md`.

## Encoding guidance

| Format | Hot path? | Notes |
|--------|-----------|-------|
| JSON | Avoid | Human / external; parse off-path with simdjson |
| Protobuf | Edge only | Excellent contracts; costlier than POD/SBE |
| **POD / packed structs** | Yes | Align carefully; document ABI |
| **SBE / FlatBuffers** | Yes | Schema evolution without parsing trees |

```cpp
// Conceptual zero-copy: interpret bytes already in a receive buffer
struct alignas(8) WireTick {
  std::uint32_t instrument;
  std::uint32_t flags;
  double px;
  double qty;
};
// static_assert(sizeof(WireTick) == 24);
// const WireTick* t = reinterpret_cast<const WireTick*>(buf);
```

## Kernel bypass (what “definitive” docs should say)

```text
NIC RX ring  --poll-->  user-space driver (DPDK/Onload)
      |
      v
  packet descriptor (zero-copy pointer into hugepage mbuf)
      |
      v
  decode POD/SBE in place → SPSC → strategy thread
```

This monorepo deliberately **does not** vendor DPDK: binary size, licenses, and hardware variance.  
A future `docs/blueprint/dpdk-interface.md` can sketch the adapter API without shipping the driver.

## Related examples

- Beast/Asio tests: `tests/test_beast_asio.cpp`  
- Binary POD discipline: companion HFT assembler repo wire format (`BIN1`)
