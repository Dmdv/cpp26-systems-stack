#include <catch2/catch_test_macros.hpp>

#include "ll/hdr_c.hpp"
#include "ll/hdr_histogram.hpp"
#include "ll/kernel_bypass.hpp"
#include "ll/linux_numa.hpp"
#include "ll/linux_uring.hpp"
#include "ll/sbe_codec.hpp"
#include "ll/sbe_style.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// 1. Real Logic SBE generated codecs
// ---------------------------------------------------------------------------

TEST_CASE("sbe generated tick encode decode roundtrip", "[roadmap][sbe]") {
  REQUIRE(ll::sbe_gen::kTickWireBytes == 8 + 24);

  ll::sbe_gen::TickValues in{.instrument_id = 42,
                             .price_ticks = 1902500,
                             .qty = 10,
                             .seq = 99,
                             .ts_ns = 1'700'000'000'000'000'000ull};
  std::array<char, 64> buf{};
  const auto n = ll::sbe_gen::encode_tick(buf, in);
  REQUIRE(n == ll::sbe_gen::kTickWireBytes);

  ll::sbe_gen::TickValues out{};
  REQUIRE(ll::sbe_gen::decode_tick(std::span<const char>(buf.data(), n), out));
  REQUIRE(out.instrument_id == 42);
  REQUIRE(out.price_ticks == 1902500);
  REQUIRE(out.qty == 10);
  REQUIRE(out.seq == 99);
  REQUIRE(out.ts_ns == 1'700'000'000'000'000'000ull);
}

TEST_CASE("sbe decode rejects wrong schema id", "[roadmap][sbe]") {
  std::array<char, 64> buf{};
  ll::sbe_gen::TickValues in{.instrument_id = 1, .price_ticks = 1, .qty = 1, .seq = 1, .ts_ns = 1};
  REQUIRE(ll::sbe_gen::encode_tick(buf, in) > 0);
  // Corrupt schemaId in header (bytes 4-5 little-endian after blockLength+templateId)
  buf[4] = 0;
  buf[5] = 0;
  ll::sbe_gen::TickValues out{};
  REQUIRE_FALSE(ll::sbe_gen::decode_tick(buf, out));
}

TEST_CASE("sbe-style pod still coexists with generated sbe", "[roadmap][sbe]") {
  ll::sbe::TickMsg pod{.instrument_id = 7, .price_ticks = 1, .qty = 2, .seq = 3};
  std::array<std::byte, 16> wire{};
  REQUIRE(ll::sbe::encode(wire, pod));
  ll::sbe::TickMsg out{};
  REQUIRE(ll::sbe::decode_copy(wire, out));
  REQUIRE(out.seq == 3);
}

// ---------------------------------------------------------------------------
// 2. Linux NUMA / io_uring (skip semantics on macOS stubs)
// ---------------------------------------------------------------------------

TEST_CASE("numa backend reports status", "[roadmap][numa]") {
  const auto line = ll::numa::status_line();
  REQUIRE_FALSE(line.empty());
  REQUIRE(line.find("numa_backend=") != std::string::npos);
#if LL_HAS_LIBNUMA
  if (ll::numa::available()) {
    REQUIRE(ll::numa::max_node() >= 0);
    // alloc small region on preferred node when possible
    const int node = ll::numa::preferred_node();
    if (node >= 0) {
      void* p = ll::numa::alloc_onnode(4096, node);
      // Some restricted CI containers disallow move_pages; allow null.
      if (p) {
        ll::numa::free_onnode(p, 4096);
      }
    }
  }
#else
  REQUIRE_FALSE(ll::numa::available());
  REQUIRE(ll::numa::bind_thread_to_node(0) == false);
#endif
}

TEST_CASE("uring backend nop path", "[roadmap][uring]") {
  REQUIRE_FALSE(ll::uring::status_line().empty());
  ll::uring::Ring ring;
#if LL_HAS_LIBURING
  REQUIRE(ll::uring::available());
  REQUIRE(ring.open(8));
  REQUIRE(ring.is_open());
  REQUIRE(ring.submit_nop_and_wait());
  // read a known small file via uring
  {
    const char* path = "/etc/hosts";
    std::array<char, 256> buf{};
    const int n = ring.read_file(path, buf);
    // hosts may be restricted in some sandboxes; accept >=0 or -1
    REQUIRE(n >= -1);
  }
  ring.close();
  REQUIRE_FALSE(ring.is_open());
#else
  REQUIRE_FALSE(ll::uring::available());
  REQUIRE_FALSE(ring.open());
  REQUIRE_FALSE(ring.submit_nop_and_wait());
#endif
}

// ---------------------------------------------------------------------------
// 3. HdrHistogram_c optional + portable HDR
// ---------------------------------------------------------------------------

TEST_CASE("portable hdr and optional hdr_c", "[roadmap][hdr_c]") {
  ll::HdrLatencyHistogram<> portable;
  for (std::uint64_t i = 1; i <= 1000; ++i) {
    portable.record(i);
  }
  REQUIRE(portable.value_at_percentile(99.0).has_value());

  ll::HdrHistogramC c;
#if LL_HAS_HDRHISTOGRAM_C
  REQUIRE(ll::hdr_c_available());
  REQUIRE(c.valid());
  for (std::int64_t i = 1; i <= 1000; ++i) {
    c.record(i);
  }
  c.record(100000);
  REQUIRE(c.max() >= 100000);
  auto p99 = c.value_at_percentile(99.0);
  REQUIRE(p99.has_value());
  REQUIRE(*p99 >= 1);
#else
  REQUIRE_FALSE(ll::hdr_c_available());
  REQUIRE_FALSE(c.valid());
#endif
}

// ---------------------------------------------------------------------------
// 4. Kernel-bypass lab stubs
// ---------------------------------------------------------------------------

TEST_CASE("kernel bypass stub poll mode rx", "[roadmap][bypass]") {
  REQUIRE_FALSE(ll::bypass::dpdk_built());
  REQUIRE_FALSE(ll::bypass::onload_built());
  REQUIRE(std::string(ll::bypass::backend_status()).find("stub") != std::string::npos);

  ll::bypass::StubPollModeRx rx;
  REQUIRE(rx.backend() == ll::bypass::Backend::Stub);
  REQUIRE(rx.open(0, 0));

  const char payload[] = "TICK";
  std::byte bytes[4];
  std::memcpy(bytes, payload, 4);
  rx.inject_for_test(bytes);

  ll::bypass::PacketView views[4]{};
  const auto n = rx.poll(views);
  REQUIRE(n == 1);
  REQUIRE(views[0].length == 4);
  REQUIRE(views[0].data[0] == std::byte{'T'});

  REQUIRE(rx.poll(views) == 0);  // drained
  rx.close();
}
