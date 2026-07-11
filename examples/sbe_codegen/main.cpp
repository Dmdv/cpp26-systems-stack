// Example: Real Logic SBE-generated Tick encode/decode (production wire path).
#include "ll/sbe_codec.hpp"

#include <array>
#include <cstdio>

int main() {
  ll::sbe_gen::TickValues in{.instrument_id = 1001,
                             .price_ticks = 1902500,
                             .qty = 50,
                             .seq = 7,
                             .ts_ns = 42};
  std::array<char, 64> wire{};
  const auto n = ll::sbe_gen::encode_tick(wire, in);
  if (n == 0) {
    return 1;
  }
  ll::sbe_gen::TickValues out{};
  if (!ll::sbe_gen::decode_tick(std::span<const char>(wire.data(), n), out)) {
    return 2;
  }
  std::printf("sbe tick wire_bytes=%zu id=%u px=%d qty=%u seq=%u ts=%llu\n", n,
              out.instrument_id, out.price_ticks, out.qty, out.seq,
              static_cast<unsigned long long>(out.ts_ns));
  std::printf("schema_id=%u template_id=%u block=%u\n",
              market::sbe::Tick::sbeSchemaId(), market::sbe::Tick::sbeTemplateId(),
              market::sbe::Tick::sbeBlockLength());
  return 0;
}
