#pragma once

// SBE-style fixed binary layout helpers (zero-copy cast over a wire buffer).
// Real Logic SBE generates codecs from XML schemas — this module teaches the
// layout contract and provides a minimal POD message for tests/examples.
// Production: generate with https://github.com/real-logic/simple-binary-encoding
//
// See docs/blueprint/02-network-ingress.md and docs/tutorials/industry-stack.md

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>

namespace ll::sbe {

// Packed little-endian tick message (16 bytes). Aligns with HFT "blast over wire" layouts.
#pragma pack(push, 1)
struct TickMsg {
  std::uint32_t instrument_id;
  std::int32_t price_ticks;   // fixed-point
  std::uint32_t qty;
  std::uint32_t seq;
};
#pragma pack(pop)

static_assert(sizeof(TickMsg) == 16);
static_assert(std::is_trivially_copyable_v<TickMsg>);

// Encode into caller-owned buffer (no allocation).
[[nodiscard]] inline bool encode(std::span<std::byte> dst, const TickMsg& msg) noexcept {
  if (dst.size() < sizeof(TickMsg)) {
    return false;
  }
  std::memcpy(dst.data(), &msg, sizeof(TickMsg));
  return true;
}

// Zero-copy view — buffer must remain live and correctly aligned for reads.
[[nodiscard]] inline const TickMsg* decode_view(std::span<const std::byte> src) noexcept {
  if (src.size() < sizeof(TickMsg)) {
    return nullptr;
  }
  return reinterpret_cast<const TickMsg*>(src.data());
}

// Safe copy-out when alignment is unknown.
[[nodiscard]] inline bool decode_copy(std::span<const std::byte> src, TickMsg& out) noexcept {
  if (src.size() < sizeof(TickMsg)) {
    return false;
  }
  std::memcpy(&out, src.data(), sizeof(TickMsg));
  return true;
}

}  // namespace ll::sbe
