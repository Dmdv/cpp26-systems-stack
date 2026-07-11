#include <catch2/catch_test_macros.hpp>

#include "ll/hdr_histogram.hpp"
#include "ll/jemalloc_util.hpp"
#include "ll/kernel_bypass.hpp"
#include "ll/pmr_arena.hpp"
#include "ll/sbe_style.hpp"
#include "ll/spsc_queue.hpp"
#include "ll/struct_pack_tick.hpp"

#include <boost/lockfree/spsc_queue.hpp>

#include <cstdint>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#if LL_HAS_MOODYCAMEL
#include "readerwriterqueue.h"
#endif

#if LL_HAS_HWLOC
#include <hwloc.h>
#endif

#if LL_HAS_FLATBUFFERS
#include "tick_generated.h"
#include <flatbuffers/flatbuffers.h>
#endif

#if LL_HAS_MIMALLOC
#include <mimalloc.h>
#endif

// ---------------------------------------------------------------------------
// std::pmr arenas
// ---------------------------------------------------------------------------

TEST_CASE("pmr monotonic arena create/release", "[industry][pmr]") {
  ll::PmrMonotonicArena arena(4096);
  auto* a = arena.create<int>(7);
  auto* b = arena.create<std::uint64_t>(99);
  REQUIRE(*a == 7);
  REQUIRE(*b == 99);
  arena.release();
  auto* c = arena.create<int>(1);
  REQUIRE(*c == 1);
}

TEST_CASE("pmr unsync pool allocate/deallocate", "[industry][pmr]") {
  ll::PmrUnsyncPool pool;
  int* p = pool.create<int>(42);
  REQUIRE(p != nullptr);
  REQUIRE(*p == 42);
  pool.destroy_and_deallocate(p);
}

// ---------------------------------------------------------------------------
// HDR-style histogram (portable)
// ---------------------------------------------------------------------------

TEST_CASE("hdr histogram tracks p99 and max", "[industry][hdr]") {
  ll::HdrLatencyHistogram<> h;
  for (std::uint64_t i = 1; i <= 1000; ++i) {
    h.record(i);
  }
  h.record(100000);  // outlier
  REQUIRE(h.count() == 1001);
  REQUIRE(h.max() == 100000);
  REQUIRE(h.min() == 1);
  auto p50 = h.value_at_percentile(50.0);
  auto p99 = h.value_at_percentile(99.0);
  REQUIRE(p50.has_value());
  REQUIRE(p99.has_value());
  REQUIRE(*p99 >= *p50);
  REQUIRE(h.max() >= *p99);
}

// ---------------------------------------------------------------------------
// SBE-style packed wire layout
// ---------------------------------------------------------------------------

TEST_CASE("sbe-style encode decode roundtrip", "[industry][sbe]") {
  ll::sbe::TickMsg in{.instrument_id = 42,
                      .price_ticks = 12345,
                      .qty = 10,
                      .seq = 7};
  alignas(16) std::byte buf[16]{};
  REQUIRE(ll::sbe::encode(buf, in));
  ll::sbe::TickMsg out{};
  REQUIRE(ll::sbe::decode_copy(buf, out));
  REQUIRE(out.instrument_id == 42);
  REQUIRE(out.price_ticks == 12345);
  REQUIRE(out.qty == 10);
  REQUIRE(out.seq == 7);
  const auto* view = ll::sbe::decode_view(buf);
  REQUIRE(view != nullptr);
  REQUIRE(view->seq == 7);
}

// ---------------------------------------------------------------------------
// Boost.Lockfree SPSC
// ---------------------------------------------------------------------------

TEST_CASE("boost lockfree spsc_queue push pop", "[industry][boost_lockfree]") {
  boost::lockfree::spsc_queue<int, boost::lockfree::capacity<64>> q;
  REQUIRE(q.push(1));
  REQUIRE(q.push(2));
  int a = 0;
  int b = 0;
  REQUIRE(q.pop(a));
  REQUIRE(q.pop(b));
  REQUIRE(a == 1);
  REQUIRE(b == 2);
}

TEST_CASE("boost lockfree spsc stress", "[industry][boost_lockfree]") {
  constexpr int N = 50000;
  boost::lockfree::spsc_queue<int, boost::lockfree::capacity<1024>> q;
  std::int64_t sum = 0;
  std::thread prod([&] {
    for (int i = 1; i <= N; ++i) {
      while (!q.push(i)) {
      }
    }
  });
  std::thread cons([&] {
    int got = 0;
    int v = 0;
    while (got < N) {
      if (q.pop(v)) {
        sum += v;
        ++got;
      }
    }
  });
  prod.join();
  cons.join();
  REQUIRE(sum == static_cast<std::int64_t>(N) * (N + 1) / 2);
}

// ---------------------------------------------------------------------------
// moodycamel::ReaderWriterQueue
// ---------------------------------------------------------------------------

#if LL_HAS_MOODYCAMEL
TEST_CASE("moodycamel readerwriterqueue stress", "[industry][moodycamel]") {
  constexpr int N = 50000;
  moodycamel::ReaderWriterQueue<int> q(1024);
  std::int64_t sum = 0;
  std::thread prod([&] {
    for (int i = 1; i <= N; ++i) {
      while (!q.try_enqueue(i)) {
      }
    }
  });
  std::thread cons([&] {
    int got = 0;
    int v = 0;
    while (got < N) {
      if (q.try_dequeue(v)) {
        sum += v;
        ++got;
      }
    }
  });
  prod.join();
  cons.join();
  REQUIRE(sum == static_cast<std::int64_t>(N) * (N + 1) / 2);
}
#endif

// ---------------------------------------------------------------------------
// hwloc topology smoke
// ---------------------------------------------------------------------------

#if LL_HAS_HWLOC
TEST_CASE("hwloc topology loads cores", "[industry][hwloc]") {
  hwloc_topology_t topo = nullptr;
  REQUIRE(hwloc_topology_init(&topo) == 0);
  REQUIRE(hwloc_topology_load(topo) == 0);
  const int n =
      hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_CORE);
  REQUIRE(n >= 1);
  hwloc_topology_destroy(topo);
}
#endif

// ---------------------------------------------------------------------------
// FlatBuffers zero-copy
// ---------------------------------------------------------------------------

#if LL_HAS_FLATBUFFERS
TEST_CASE("flatbuffers tick build and access", "[industry][flatbuffers]") {
  flatbuffers::FlatBufferBuilder fbb(256);
  auto sym = fbb.CreateString("AAPL");
  auto tick = market::CreateTick(fbb, sym, 190.5, 100.0, 123456789ull, 7);
  fbb.Finish(tick);

  const auto* got = market::GetTick(fbb.GetBufferPointer());
  REQUIRE(got != nullptr);
  REQUIRE(got->symbol()->str() == "AAPL");
  REQUIRE(got->price() == 190.5);
  REQUIRE(got->qty() == 100.0);
  REQUIRE(got->ts_ns() == 123456789ull);
  REQUIRE(got->seq() == 7);
}
#endif

// ---------------------------------------------------------------------------
// mimalloc off-critical-path allocator
// ---------------------------------------------------------------------------

#if LL_HAS_MIMALLOC
TEST_CASE("mimalloc allocate free", "[industry][mimalloc]") {
  void* p = mi_malloc(128);
  REQUIRE(p != nullptr);
  std::memset(p, 0xAB, 128);
  mi_free(p);
}
#endif

// ---------------------------------------------------------------------------
// jemalloc off-critical-path allocator
// ---------------------------------------------------------------------------

#if LL_HAS_JEMALLOC
TEST_CASE("jemalloc allocate free and version", "[industry][jemalloc]") {
  REQUIRE(ll::je::available());
  void* p = ll::je::malloc_bytes(256);
  REQUIRE(p != nullptr);
  std::memset(p, 0xCD, 256);
  ll::je::free_bytes(p);
  REQUIRE_FALSE(ll::je::version_string().empty());
}
#endif

// ---------------------------------------------------------------------------
// struct_pack (yalantinglibs)
// ---------------------------------------------------------------------------

#if LL_HAS_STRUCT_PACK
TEST_CASE("struct_pack tick serialize deserialize", "[industry][struct_pack]") {
  REQUIRE(ll::sp::available());
  ll::sp::Tick in{.instrument_id = 7,
                  .price_ticks = 1902500,
                  .qty = 3,
                  .seq = 99,
                  .ts_ns = 42};
  auto buf = ll::sp::serialize(in);
  REQUIRE_FALSE(buf.empty());
  ll::sp::Tick out{};
  REQUIRE(ll::sp::deserialize(buf, out));
  REQUIRE(out == in);
}
#endif

// ---------------------------------------------------------------------------
// folly::ProducerConsumerQueue (when Folly is installed)
// ---------------------------------------------------------------------------

#if LL_HAS_FOLLY_PCQ
#include <folly/ProducerConsumerQueue.h>

TEST_CASE("folly ProducerConsumerQueue in industry suite", "[industry][folly_pcq]") {
  folly::ProducerConsumerQueue<int> q(64);
  REQUIRE(q.write(11));
  int v = 0;
  REQUIRE(q.read(v));
  REQUIRE(v == 11);
}
#endif

// ---------------------------------------------------------------------------
// Kernel-bypass capabilities (DPDK/Onload detect or stub)
// ---------------------------------------------------------------------------

TEST_CASE("kernel bypass capabilities and stub rx", "[industry][bypass]") {
  const auto caps = ll::bypass::capabilities();
  REQUIRE(caps.stub);
  // On typical developer laptops both remain false (no NIC SDK).
  (void)caps.dpdk;
  (void)caps.onload;
  REQUIRE_FALSE(ll::bypass::preferred_backend_name().empty());

  auto rx = ll::bypass::make_default_rx();
  REQUIRE(rx.open(0, 0));
  std::byte payload[4]{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
  rx.inject_for_test(payload);
  ll::bypass::PacketView views[2]{};
  REQUIRE(rx.poll(views) == 1);
  REQUIRE(views[0].length == 4);
}

// Cross-check: ll SPSC still coexists with industry queues
TEST_CASE("ll spsc still works alongside industry suite", "[industry][ll]") {
  ll::SpscQueue<int, 8> q;
  REQUIRE(q.try_push(3));
  auto v = q.try_pop();
  REQUIRE(v.has_value());
  REQUIRE(*v == 3);
}
