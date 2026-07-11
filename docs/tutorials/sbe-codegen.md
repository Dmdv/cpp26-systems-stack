# Tutorial: Real Logic SBE codegen (production wire path)

Simple Binary Encoding (SBE) is the industry standard for **copy-free, fixed-layout**
market data codecs. This repo ships:

1. An XML schema — `schemas/sbe-market-data-schema.xml`
2. Generated C++ headers — `generated/sbe/market_sbe/{Tick,MessageHeader}.h`
3. Thin helpers — `include/ll/sbe_codec.hpp`
4. Tests — `[roadmap][sbe]` · example — `example_sbe_codegen`

## Why not hand-rolled POD forever?

`ll::sbe::TickMsg` (SBE-*style*) teaches layout. Production venues version schemas,
add optional fields, and require **message headers** (blockLength, templateId,
schemaId, version). Real Logic’s generator emits bounds-checked accessors that
match the official wire contract.

## Regenerate codecs

```bash
# Requires Java 11+
./scripts/generate_sbe.sh
# Optional: SBE_VERSION=1.34.1 ./scripts/generate_sbe.sh
git add schemas/sbe-market-data-schema.xml generated/sbe/
```

The script downloads `sbe-all` from Maven Central into `.tools/` (gitignored).

## Schema sketch

```xml
<sbe:message name="Tick" id="1">
  <field name="instrumentId" id="1" type="InstrumentId"/>  <!-- uint32 -->
  <field name="priceTicks"   id="2" type="PriceTicks"/>    <!-- int32  -->
  <field name="qty"          id="3" type="Quantity"/>
  <field name="seq"          id="4" type="Sequence"/>
  <field name="tsNs"         id="5" type="TimestampNs"/>  <!-- uint64 -->
</sbe:message>
```

Wire size = **8-byte header + 24-byte body** (`ll::sbe_gen::kTickWireBytes`).

## Encode / decode in application code

```cpp
#include "ll/sbe_codec.hpp"

ll::sbe_gen::TickValues in{.instrument_id = 1001, .price_ticks = 1902500,
                           .qty = 50, .seq = 7, .ts_ns = 42};
char buf[64];
auto n = ll::sbe_gen::encode_tick(buf, in);

ll::sbe_gen::TickValues out;
ll::sbe_gen::decode_tick(std::span<const char>(buf, n), out);
```

Or use generated types directly:

```cpp
#include "Tick.h"
market::sbe::Tick tick;
tick.wrapAndApplyHeader(buf, 0, sizeof buf);
tick.instrumentId(1001).priceTicks(1902500).qty(50).seq(7).tsNs(42);
```

## Run

```bash
cmake --build build --target example_sbe_codegen test_roadmap_stack
./build/example_sbe_codegen
ctest --test-dir build -R 'roadmap.*sbe'
```

## Production checklist

| Step | Action |
|------|--------|
| 1 | Copy venue XML schemas into `schemas/` |
| 2 | Run `generate_sbe.sh` (or CI job) on schema change |
| 3 | Commit generated headers for hermetic builds (no Java in default CI) |
| 4 | Reject `schemaId` / `templateId` mismatches on decode (see tests) |
| 5 | Keep JSON/protobuf **off** the exchange-facing hot path |

## Related

- Compact POD demo: `ll/sbe_style.hpp` · `example_sbe_style`
- FlatBuffers alternative: `schemas/tick.fbs`
- Kernel-bypass delivery of SBE buffers: `docs/tutorials/kernel-bypass-lab.md`
