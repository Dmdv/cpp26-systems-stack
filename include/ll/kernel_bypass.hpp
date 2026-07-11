#pragma once

// Kernel-bypass / poll-mode NIC abstraction (DPDK / OpenOnload / EF_VI).
// This is an API *contract* and lab checklist — not a linked driver product.
// Production desks implement these interfaces against vendor SDKs on dedicated hardware.
//
// See docs/tutorials/kernel-bypass-lab.md

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

#ifndef LL_HAS_DPDK
#define LL_HAS_DPDK 0
#endif
#ifndef LL_HAS_ONLOAD
#define LL_HAS_ONLOAD 0
#endif

namespace ll::bypass {

enum class Backend : std::uint8_t {
  None = 0,
  Stub,       // compile-time placeholder used in this repo's tests
  Dpdk,       // Intel DPDK
  OpenOnload, // Solarflare/Xilinx OpenOnload
  EfVi,       // Solarflare EF_VI (lower level than Onload sockets)
};

[[nodiscard]] inline constexpr bool dpdk_built() noexcept { return LL_HAS_DPDK != 0; }
[[nodiscard]] inline constexpr bool onload_built() noexcept { return LL_HAS_ONLOAD != 0; }

// One received packet view (zero-copy when backend supports it).
struct PacketView {
  const std::byte* data{nullptr};
  std::size_t length{0};
  std::uint64_t hw_timestamp_ns{0};  // 0 if unavailable
  std::uint16_t port_id{0};
  std::uint16_t queue_id{0};
};

// Poll-mode RX interface — hot path should call poll() in a tight loop on a pinned core.
class IPollModeRx {
 public:
  virtual ~IPollModeRx() = default;

  [[nodiscard]] virtual Backend backend() const noexcept = 0;
  [[nodiscard]] virtual std::string_view name() const noexcept = 0;

  // Initialize port/queue. Returns false on config failure.
  virtual bool open(std::uint16_t port_id, std::uint16_t queue_id) = 0;
  virtual void close() noexcept = 0;

  // Non-blocking: fill up to `out.size()` packet views. Returns count filled.
  // Implementations must not allocate on the hot path.
  virtual std::size_t poll(std::span<PacketView> out) noexcept = 0;
};

// Stub backend for unit tests / documentation — never talks to a real NIC.
class StubPollModeRx final : public IPollModeRx {
 public:
  [[nodiscard]] Backend backend() const noexcept override { return Backend::Stub; }
  [[nodiscard]] std::string_view name() const noexcept override { return "stub-poll-mode-rx"; }

  bool open(std::uint16_t port_id, std::uint16_t queue_id) override {
    port_ = port_id;
    queue_ = queue_id;
    open_ = true;
    return true;
  }

  void close() noexcept override { open_ = false; }

  // Inject a synthetic packet for tests (off hot-path in real systems).
  void inject_for_test(std::span<const std::byte> payload) {
    if (payload.size() > sizeof(slot_)) {
      return;
    }
    std::memcpy(slot_, payload.data(), payload.size());
    len_ = payload.size();
    pending_ = true;
  }

  std::size_t poll(std::span<PacketView> out) noexcept override {
    if (!open_ || !pending_ || out.empty()) {
      return 0;
    }
    out[0] = PacketView{
        .data = reinterpret_cast<const std::byte*>(slot_),
        .length = len_,
        .hw_timestamp_ns = 0,
        .port_id = port_,
        .queue_id = queue_,
    };
    pending_ = false;
    return 1;
  }

 private:
  bool open_{false};
  bool pending_{false};
  std::uint16_t port_{0};
  std::uint16_t queue_{0};
  std::size_t len_{0};
  alignas(64) std::byte slot_[2048]{};
};

[[nodiscard]] inline const char* backend_status() noexcept {
  if (dpdk_built()) {
    return "dpdk";
  }
  if (onload_built()) {
    return "onload";
  }
  return "stub-only (no NIC SDK linked)";
}

}  // namespace ll::bypass
