#pragma once

// Thin helpers around Real Logic SBE-generated codecs (market.sbe.Tick).
// Schema: schemas/sbe-market-data-schema.xml
// Regenerate: scripts/generate_sbe.sh
// Generated headers: generated/sbe/market_sbe/

#include "Tick.h"

#include <cstddef>
#include <cstdint>
#include <span>

namespace ll::sbe_gen {

// Wire size: 8-byte messageHeader + 24-byte Tick body.
inline constexpr std::size_t kTickWireBytes =
    market::sbe::Tick::sbeBlockAndHeaderLength();

struct TickValues {
  std::uint32_t instrument_id{0};
  std::int32_t price_ticks{0};
  std::uint32_t qty{0};
  std::uint32_t seq{0};
  std::uint64_t ts_ns{0};
};

// Encode header + body into caller-owned buffer. Returns bytes written, or 0.
[[nodiscard]] inline std::size_t encode_tick(std::span<char> dst, const TickValues& v) {
  if (dst.size() < kTickWireBytes) {
    return 0;
  }
  market::sbe::Tick tick;
  tick.wrapAndApplyHeader(dst.data(), 0, dst.size());
  tick.instrumentId(v.instrument_id)
      .priceTicks(v.price_ticks)
      .qty(v.qty)
      .seq(v.seq)
      .tsNs(v.ts_ns);
  return kTickWireBytes;
}

// Decode from buffer that starts with messageHeader. Returns false on short buffer / schema mismatch.
[[nodiscard]] inline bool decode_tick(std::span<const char> src, TickValues& out) {
  if (src.size() < market::sbe::MessageHeader::encodedLength()) {
    return false;
  }
  // MessageHeader needs non-const buffer API in generated code — cast is safe for read-only decode.
  char* buf = const_cast<char*>(src.data());
  market::sbe::MessageHeader hdr(buf, 0, src.size(), market::sbe::Tick::sbeSchemaVersion());
  if (hdr.schemaId() != market::sbe::Tick::sbeSchemaId() ||
      hdr.templateId() != market::sbe::Tick::sbeTemplateId()) {
    return false;
  }
  market::sbe::Tick tick;
  tick.wrapForDecode(buf, market::sbe::MessageHeader::encodedLength(), hdr.blockLength(),
                     hdr.version(), src.size());
  out.instrument_id = tick.instrumentId();
  out.price_ticks = tick.priceTicks();
  out.qty = tick.qty();
  out.seq = tick.seq();
  out.ts_ns = tick.tsNs();
  return true;
}

}  // namespace ll::sbe_gen
