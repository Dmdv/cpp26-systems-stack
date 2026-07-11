// IDE index translation unit — not part of the runtime product.
//
// CLion / clangd index symbols from TUs that are part of a CMake target.
// This file pulls in every public stack header (ll::*, SBE, industry mesh)
// so go-to-definition, completion, and type hints work for the full surface.
//
// Build: cmake --build --preset clion-debug --target ide_index
// Or open project in CLion with preset "clion-debug" (target listed in build preset).

#include "ll/industry.hpp"

#if LL_HAS_SBE_CODEGEN
#include "ll/sbe_codec.hpp"
#include "MessageHeader.h"
#include "Tick.h"
#endif

// Library mesh headers (same set exercised by lib_smoke / test_libs)
#include <asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <simdjson.h>
#include <taskflow/taskflow.hpp>
#include <tbb/parallel_reduce.h>
#include <tbb/blocked_range.h>
#include <Eigen/Dense>
#include <range/v3/view/iota.hpp>
#include <absl/strings/str_cat.h>

#if LL_HAS_MOODYCAMEL
#include "readerwriterqueue.h"
#endif

#if LL_HAS_HWLOC
#include <hwloc.h>
#endif

#if LL_HAS_MIMALLOC
#include <mimalloc.h>
#endif

#if LL_HAS_HDRHISTOGRAM_C
#include <hdr/hdr_histogram.h>
#endif

#include <atomic>
#include <cstdint>
#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Touch a representative sample of types so they stay in the index even under
// aggressive "unused" pruning in some IDE modes.
namespace ide_index_detail {

[[maybe_unused]] void touch_ll_types() {
  ll::SpscQueue<int, 8> q;
  (void)q.try_push(1);
  ll::Arena arena(256);
  (void)arena.allocate(16);
  ll::PmrMonotonicArena pmr(256);
  (void)pmr.resource();
  ll::HdrLatencyHistogram<> hist;
  hist.record(1);
  ll::HdrHistogramC hc;
  (void)hc.valid();
  ll::bypass::StubPollModeRx rx;
  (void)rx.name();
  (void)ll::numa::backend();
  (void)ll::uring::backend();
  (void)ll::read_tsc();
  (void)ll::steady_ns();
  (void)ll::kCacheLine;
#if LL_HAS_SBE_CODEGEN
  ll::sbe_gen::TickValues tv{};
  char buf[64]{};
  (void)ll::sbe_gen::encode_tick(buf, tv);
#endif
  ll::sbe::TickMsg pod{};
  (void)pod.seq;
}

}  // namespace ide_index_detail

int main() {
  ide_index_detail::touch_ll_types();
  const auto s = fmt::format("ide_index ok c++{}", 26);
  spdlog::info("{}", s);
  return s.empty() ? 1 : 0;
}
