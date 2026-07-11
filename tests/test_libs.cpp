#include <catch2/catch_test_macros.hpp>

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <absl/strings/str_cat.h>
#include <tbb/parallel_reduce.h>
#include <tbb/blocked_range.h>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/numeric/accumulate.hpp>
#include <nlohmann/json.hpp>
#include <simdjson.h>
#include <Eigen/Dense>
#include <grpcpp/grpcpp.h>
#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>

#include "smoke.pb.h"

#include <atomic>
#include <string>

// Folly / HPX have dedicated test targets when LIB_SMOKE_WITH_* is ON.

TEST_CASE("fmt formats values", "[fmt]") {
  REQUIRE(fmt::format("{}-{}", "lib", 26) == "lib-26");
  REQUIRE(FMT_VERSION > 0);
}

TEST_CASE("spdlog is usable", "[spdlog]") {
  spdlog::set_level(spdlog::level::warn);
  spdlog::warn("spdlog smoke");
  REQUIRE(SPDLOG_VER_MAJOR >= 1);
}

TEST_CASE("abseil StrCat", "[abseil]") {
  REQUIRE(absl::StrCat("a", "b", 1) == "ab1");
}

TEST_CASE("oneTBB parallel_reduce", "[tbb]") {
  const int n = 5000;
  const int sum = tbb::parallel_reduce(
      tbb::blocked_range<int>(1, n + 1), 0,
      [](const tbb::blocked_range<int>& r, int local) {
        for (int i = r.begin(); i != r.end(); ++i) local += i;
        return local;
      },
      std::plus<>{});
  REQUIRE(sum == n * (n + 1) / 2);
}

TEST_CASE("range-v3 pipeline", "[range-v3]") {
  using namespace ranges;
  auto vals = views::iota(1, 11) | views::transform([](int x) { return x * 2; });
  REQUIRE(accumulate(vals, 0) == 110);
}

TEST_CASE("nlohmann-json", "[json]") {
  nlohmann::json j = {{"a", 1}, {"b", true}};
  REQUIRE(j["a"] == 1);
  REQUIRE(j.at("b") == true);
  REQUIRE(j.dump().find("\"a\":1") != std::string::npos);
}

TEST_CASE("simdjson parse", "[simdjson]") {
  const std::string body = R"({"x":10,"y":[1,2,3]})";
  simdjson::ondemand::parser parser;
  simdjson::padded_string pad{body};
  auto doc = parser.iterate(pad);
  int64_t x = doc["x"];
  REQUIRE(x == 10);
  simdjson::ondemand::array arr = doc["y"];
  int sum = 0;
  for (auto v : arr) {
    int64_t n = v;
    sum += static_cast<int>(n);
  }
  REQUIRE(sum == 6);
}

TEST_CASE("Eigen linear algebra", "[eigen]") {
  Eigen::Matrix3d I = Eigen::Matrix3d::Identity();
  Eigen::Vector3d v(1, 2, 3);
  REQUIRE((I * v).isApprox(v));
}

TEST_CASE("protobuf generated message", "[protobuf]") {
  smoke::SmokePing p;
  p.set_name("catch2");
  p.set_count(1);
  p.add_tags("test");
  std::string out;
  REQUIRE(p.SerializeToString(&out));
  smoke::SmokePing q;
  REQUIRE(q.ParseFromString(out));
  REQUIRE(q.name() == "catch2");
  REQUIRE(q.tags(0) == "test");
}

TEST_CASE("gRPC version string is non-empty", "[grpc]") {
  REQUIRE_FALSE(grpc::Version().empty());
}

TEST_CASE("stdexec static_thread_pool", "[stdexec]") {
  exec::static_thread_pool pool{2};
  auto sched = pool.get_scheduler();
  std::atomic<int> n{0};
  auto snd = stdexec::schedule(sched) | stdexec::then([&] { n.store(99); });
  auto result = stdexec::sync_wait(std::move(snd));
  REQUIRE(result.has_value());
  REQUIRE(n.load() == 99);
}
