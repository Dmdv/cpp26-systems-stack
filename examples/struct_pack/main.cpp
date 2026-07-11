// Example: yalantinglibs struct_pack serialize/deserialize for a market tick.
#include "ll/struct_pack_tick.hpp"

#include <cstdio>

int main() {
  if (!ll::sp::available()) {
    std::printf("struct_pack: not enabled (STACK_WITH_STRUCT_PACK=OFF or fetch failed)\n");
    return 0;
  }
  ll::sp::Tick t{.instrument_id = 1001,
                 .price_ticks = 1902500,
                 .qty = 10,
                 .seq = 1,
                 .ts_ns = 99};
  auto buf = ll::sp::serialize(t);
  ll::sp::Tick out{};
  if (!ll::sp::deserialize(buf, out)) {
    std::printf("struct_pack: deserialize failed\n");
    return 1;
  }
  std::printf("struct_pack ok bytes=%zu id=%u px=%d qty=%u seq=%u\n", buf.size(),
              out.instrument_id, out.price_ticks, out.qty, out.seq);
  return 0;
}
