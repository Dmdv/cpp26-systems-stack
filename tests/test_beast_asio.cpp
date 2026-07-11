#include <catch2/catch_test_macros.hpp>

#include <asio.hpp>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <atomic>
#include <chrono>
#include <string>

namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;

TEST_CASE("standalone Asio posts work to io_context", "[asio]") {
  asio::io_context ioc;
  std::atomic<int> hits{0};

  asio::post(ioc, [&] { hits.fetch_add(1); });
  asio::post(ioc, [&] { hits.fetch_add(1); });
  const auto n = ioc.run();

  REQUIRE(n >= 2);
  REQUIRE(hits.load() == 2);
}

TEST_CASE("Boost.Asio steady_timer fires", "[asio][boost]") {
  net::io_context ioc;
  net::steady_timer timer(ioc, std::chrono::milliseconds(1));
  std::atomic<bool> fired{false};

  timer.async_wait([&](const beast::error_code& ec) {
    REQUIRE_FALSE(ec);
    fired.store(true);
  });
  ioc.run();
  REQUIRE(fired.load());
}

TEST_CASE("Boost.Beast builds and serializes HTTP request", "[beast]") {
  http::request<http::string_body> req{http::verb::post, "/v1/smoke", 11};
  req.set(http::field::host, "127.0.0.1");
  req.set(http::field::content_type, "application/json");
  req.body() = R"({"ok":true})";
  req.prepare_payload();

  REQUIRE(req.method() == http::verb::post);
  REQUIRE(req.target() == "/v1/smoke");
  REQUIRE(req.body() == R"({"ok":true})");
  REQUIRE(req[http::field::host] == "127.0.0.1");
  REQUIRE(BOOST_BEAST_VERSION > 0);
}

TEST_CASE("Boost.Beast parses HTTP response buffer", "[beast]") {
  const std::string raw =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/plain\r\n"
      "Content-Length: 5\r\n"
      "\r\n"
      "hello";

  http::response_parser<http::string_body> parser;
  parser.eager(true);
  beast::error_code ec;
  const auto used =
      parser.put(net::const_buffer(raw.data(), raw.size()), ec);
  REQUIRE_FALSE(ec);
  REQUIRE(used == raw.size());
  REQUIRE(parser.is_done());

  auto const& res = parser.get();
  REQUIRE(res.result() == http::status::ok);
  REQUIRE(res.body() == "hello");
}
