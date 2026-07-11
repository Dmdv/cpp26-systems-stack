#pragma once

// Thin helpers around yalantinglibs struct_pack for a market tick POD.
// Enabled when LL_HAS_STRUCT_PACK=1 (CMake FetchContent).
// See docs/tutorials/industry-stack.md

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#ifndef LL_HAS_STRUCT_PACK
#define LL_HAS_STRUCT_PACK 0
#endif

#if LL_HAS_STRUCT_PACK
#include <ylt/struct_pack.hpp>
#endif

namespace ll::sp {

// Aggregated type for compile-time reflection (struct_pack requirement).
struct Tick {
  std::uint32_t instrument_id{0};
  std::int32_t price_ticks{0};
  std::uint32_t qty{0};
  std::uint32_t seq{0};
  std::uint64_t ts_ns{0};

  auto operator==(const Tick& o) const -> bool {
    return instrument_id == o.instrument_id && price_ticks == o.price_ticks &&
           qty == o.qty && seq == o.seq && ts_ns == o.ts_ns;
  }
};

#if LL_HAS_STRUCT_PACK

[[nodiscard]] inline std::vector<char> serialize(const Tick& t) {
  return struct_pack::serialize(t);
}

[[nodiscard]] inline bool deserialize(const std::vector<char>& buf, Tick& out) {
  auto r = struct_pack::deserialize<Tick>(buf);
  if (!r) {
    return false;
  }
  out = std::move(r.value());
  return true;
}

[[nodiscard]] inline bool available() noexcept { return true; }

#else

[[nodiscard]] inline std::vector<char> serialize(const Tick&) { return {}; }

[[nodiscard]] inline bool deserialize(const std::vector<char>&, Tick&) { return false; }

[[nodiscard]] inline bool available() noexcept { return false; }

#endif

}  // namespace ll::sp
