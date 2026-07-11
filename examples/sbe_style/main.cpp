// Example: packed SBE-style wire message (zero-copy view after memcpy into buffer).
#include "ll/sbe_style.hpp"

#include <array>
#include <cstdio>

int main() {
  ll::sbe::TickMsg msg{.instrument_id = 1001,
                       .price_ticks = 1902500,
                       .qty = 50,
                       .seq = 42};
  std::array<std::byte, sizeof(ll::sbe::TickMsg)> wire{};
  if (!ll::sbe::encode(wire, msg)) {
    return 1;
  }
  const auto* view = ll::sbe::decode_view(wire);
  std::printf("tick id=%u px_ticks=%d qty=%u seq=%u bytes=%zu\n", view->instrument_id,
              view->price_ticks, view->qty, view->seq, sizeof(ll::sbe::TickMsg));
  return 0;
}
