// lib_smoke — one-shot verification of every Homebrew C++ library from the
// install table (plus stdexec via FetchContent and a tiny protobuf message).

#if defined(LIB_SMOKE_HAS_FOLLY) && LIB_SMOKE_HAS_FOLLY
#include "folly_compat.hpp"
#endif

#include <asio.hpp>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/version.hpp>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include <absl/strings/str_cat.h>
#include <absl/strings/str_format.h>

#include <tbb/parallel_reduce.h>
#include <tbb/blocked_range.h>

#include <taskflow/taskflow.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/numeric/accumulate.hpp>

#include <nlohmann/json.hpp>
#include <simdjson.h>
#include <Eigen/Dense>

#include <grpcpp/grpcpp.h>
#include <google/protobuf/stubs/common.h>

#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>

#if LIB_SMOKE_HAS_FOLLY
#include <folly/FBString.h>
#include <folly/Optional.h>
#include <folly/String.h>
#include <folly/container/F14Map.h>
#endif

#if LIB_SMOKE_HAS_HPX
#include <hpx/hpx_main.hpp>
#include <hpx/future.hpp>
#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#endif

#include "smoke.pb.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;

static int g_passed = 0;
static int g_failed = 0;

#define CHECK(cond, name)                                                      \
  do {                                                                         \
    if (cond) {                                                                \
      fmt::print("  [PASS] {}\n", name);                                       \
      ++g_passed;                                                              \
    } else {                                                                   \
      fmt::print("  [FAIL] {}\n", name);                                       \
      ++g_failed;                                                              \
    }                                                                          \
  } while (0)

static void smoke_fmt_spdlog() {
  fmt::print("== fmt + spdlog ==\n");
  const auto s = fmt::format("fmt {} works", FMT_VERSION);
  CHECK(s.find("works") != std::string::npos, "fmt::format");
  spdlog::set_level(spdlog::level::info);
  spdlog::info("spdlog ok (version major={})", SPDLOG_VER_MAJOR);
  CHECK(true, "spdlog");
}

static void smoke_abseil() {
  fmt::print("== abseil ==\n");
  const auto cat = absl::StrCat("hello", "-", 42);
  CHECK(cat == "hello-42", "absl::StrCat");
  const auto formatted = absl::StrFormat("count=%d", 7);
  CHECK(formatted == "count=7", "absl::StrFormat");
}

static void smoke_boost_beast_asio() {
  fmt::print("== boost (Beast + Asio) + standalone asio ==\n");
  CHECK(BOOST_VERSION >= 109000, "Boost version >= 1.90");
  CHECK(BOOST_BEAST_VERSION > 0, "Boost.Beast header");

  // Standalone Asio: post work and run the io_context.
  {
    asio::io_context ioc;
    std::atomic<int> hit{0};
    asio::post(ioc, [&] { hit.fetch_add(1); });
    ioc.run();
    CHECK(hit.load() == 1, "standalone Asio post/run");
  }

  // Boost.Asio timer
  {
    net::io_context ioc;
    net::steady_timer timer(ioc, std::chrono::milliseconds(1));
    std::atomic<bool> fired{false};
    timer.async_wait([&](const beast::error_code& ec) {
      if (!ec) fired.store(true);
    });
    ioc.run();
    CHECK(fired.load(), "Boost.Asio steady_timer");
  }

  // Beast: serialize a tiny HTTP request (no network).
  {
    http::request<http::string_body> req{http::verb::get, "/smoke", 11};
    req.set(http::field::host, "localhost");
    req.set(http::field::user_agent, "lib_smoke");
    req.body() = "ping";
    req.prepare_payload();
    CHECK(req.method() == http::verb::get, "Beast HTTP request");
    CHECK(req.body() == "ping", "Beast HTTP body");
  }
}

static void smoke_tbb() {
  fmt::print("== oneTBB ==\n");
  const int n = 10'000;
  const auto sum = tbb::parallel_reduce(
      tbb::blocked_range<int>(0, n), 0,
      [](const tbb::blocked_range<int>& r, int local) {
        for (int i = r.begin(); i != r.end(); ++i) local += i;
        return local;
      },
      [](int a, int b) { return a + b; });
  const int expected = (n - 1) * n / 2;
  CHECK(sum == expected, "tbb::parallel_reduce sum");
}

static void smoke_taskflow() {
  fmt::print("== Taskflow ==\n");
  tf::Taskflow flow;
  tf::Executor executor;
  std::atomic<int> acc{0};
  auto A = flow.emplace([&] { acc.fetch_add(1); });
  auto B = flow.emplace([&] { acc.fetch_add(10); });
  auto C = flow.emplace([&] { acc.fetch_add(100); });
  A.precede(B);
  B.precede(C);
  executor.run(flow).wait();
  CHECK(acc.load() == 111, "Taskflow A->B->C graph");
}

static void smoke_range_v3() {
  fmt::print("== range-v3 ==\n");
  using namespace ranges;
  auto squares = views::iota(1, 6) | views::transform([](int x) { return x * x; });
  const int sum = accumulate(squares, 0);
  CHECK(sum == 55, "range-v3 iota|transform|accumulate");
}

static void smoke_json() {
  fmt::print("== nlohmann-json + simdjson ==\n");
  nlohmann::json j = {{"lib", "nlohmann"}, {"ok", true}, {"n", 3}};
  CHECK(j["ok"] == true, "nlohmann::json round-trip");

  const std::string payload = R"({"lib":"simdjson","ok":true,"n":3})";
  simdjson::ondemand::parser parser;
  simdjson::padded_string pad{payload};
  auto doc = parser.iterate(pad);
  std::string_view lib = doc["lib"];
  bool ok = doc["ok"];
  CHECK(lib == "simdjson" && ok, "simdjson ondemand parse");
}

static void smoke_eigen() {
  fmt::print("== Eigen ==\n");
  Eigen::Matrix2d m;
  m << 1, 2, 3, 4;
  Eigen::Vector2d v(1, 1);
  Eigen::Vector2d r = m * v;
  CHECK(r(0) == 3.0 && r(1) == 7.0, "Eigen Matrix*Vector");
}

static void smoke_protobuf_grpc() {
  fmt::print("== protobuf + gRPC ==\n");
  smoke::SmokePing ping;
  ping.set_name("lib_smoke");
  ping.set_count(2);
  ping.add_tags("homebrew");
  ping.add_tags("cpp26");
  std::string bytes;
  CHECK(ping.SerializeToString(&bytes) && !bytes.empty(), "protobuf serialize");
  smoke::SmokePing restored;
  CHECK(restored.ParseFromString(bytes), "protobuf parse");
  CHECK(restored.name() == "lib_smoke" && restored.tags_size() == 2,
        "protobuf fields");

  // Link against gRPC without starting a server.
  const auto ver = grpc::Version();
  CHECK(!ver.empty(), fmt::format("gRPC version ({})", ver));
  (void)google::protobuf::internal::VersionString(GOOGLE_PROTOBUF_VERSION);
}

static void smoke_stdexec() {
  fmt::print("== stdexec (FetchContent) ==\n");
  exec::static_thread_pool pool{2};
  auto sched = pool.get_scheduler();

  std::atomic<int> value{0};
  auto work = stdexec::schedule(sched)
            | stdexec::then([&] { value.store(42); });
  stdexec::sync_wait(std::move(work));
  CHECK(value.load() == 42, "stdexec schedule|then|sync_wait");
}

#if LIB_SMOKE_HAS_FOLLY
static void smoke_folly() {
  fmt::print("== Folly (Homebrew) ==\n");
  folly::fbstring s = "hello";
  s += "-folly";
  CHECK(s == "hello-folly", "folly::fbstring");

  folly::Optional<int> opt = 7;
  CHECK(opt.has_value() && *opt == 7, "folly::Optional");

  folly::F14FastMap<std::string, int> m;
  m["a"] = 1;
  m["b"] = 2;
  CHECK(m.size() == 2 && m["a"] == 1, "folly::F14FastMap");

  const auto joined = folly::join(",", std::vector<std::string>{"x", "y", "z"});
  CHECK(joined == "x,y,z", "folly::join");
}
#endif

#if LIB_SMOKE_HAS_HPX
static void smoke_hpx() {
  fmt::print("== HPX (local install) ==\n");
  auto f = hpx::make_ready_future(21).then([](hpx::future<int> r) {
    return r.get() * 2;
  });
  CHECK(f.get() == 42, "hpx future then");

  std::vector<int> data(1000);
  std::iota(data.begin(), data.end(), 1);
  const auto sum = hpx::reduce(hpx::execution::par, data.begin(), data.end(), 0);
  CHECK(sum == 1000 * 1001 / 2, "hpx::reduce par");
}
#endif

int main() {
  fmt::print("lib_smoke — C++26 library installation verifier\n");
  fmt::print("compiler: Apple/LLVM, standard c++26\n");
  fmt::print("optional: Folly={} HPX={}\n\n",
             LIB_SMOKE_HAS_FOLLY ? "ON" : "OFF",
             LIB_SMOKE_HAS_HPX ? "ON" : "OFF");

  smoke_fmt_spdlog();
  smoke_abseil();
  smoke_boost_beast_asio();
  smoke_tbb();
  smoke_taskflow();
  smoke_range_v3();
  smoke_json();
  smoke_eigen();
  smoke_protobuf_grpc();
  smoke_stdexec();
#if LIB_SMOKE_HAS_FOLLY
  smoke_folly();
#endif
#if LIB_SMOKE_HAS_HPX
  smoke_hpx();
#endif

  fmt::print("\nSummary: {} passed, {} failed\n", g_passed, g_failed);
  return g_failed == 0 ? 0 : 1;
}
