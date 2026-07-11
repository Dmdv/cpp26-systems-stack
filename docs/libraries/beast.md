# Boost.Beast (HTTP + WebSocket)

**What it is:** HTTP and WebSocket library built on **Boost.Asio** (not standalone Asio).  
**Install:** Comes with Homebrew `boost` (Beast is header-only).  
**Smoke:** `tests/test_beast_asio.cpp` tags `[beast]`  
**Related:** [asio.md](./asio.md) for `io_context`, timers, executors

## Why Beast?

| Strength | Use |
|----------|-----|
| **HTTP/1.1** | REST clients/servers without libcurl-in-process |
| **WebSocket** | Streaming market data / notifications |
| **Zero-copy-ish buffers** | `flat_buffer`, body types |
| **Same model as Asio** | async handlers, strands, SSL via Asio |

Beast is **not** HTTP/2 or gRPC — for RPC contracts prefer **protobuf + gRPC**.

## Mental model

```text
  TCP socket (Asio)
       │
       ▼
  Beast HTTP  ──  request/response messages + parsers/serializers
       │
       ▼
  Beast WebSocket  ──  frames over the upgraded connection
```

Namespaces (this project’s style):

```cpp
namespace beast = boost::beast;
namespace http  = beast::http;
namespace websocket = beast::websocket;
namespace net   = boost::asio;
using tcp = net::ip::tcp;
```

## HTTP — messages without I/O

### Build a request

```cpp
#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

http::request<http::string_body> req{http::verb::post, "/v1/smoke", 11};
req.set(http::field::host, "127.0.0.1");
req.set(http::field::content_type, "application/json");
req.body() = R"({"ok":true})";
req.prepare_payload();  // sets Content-Length
```

From `tests/test_beast_asio.cpp`.

### Parse a response from bytes

```cpp
#include <boost/beast/http.hpp>
#include <boost/asio/buffer.hpp>
#include <string>

namespace http  = boost::beast::http;
namespace beast = boost::beast;
namespace net   = boost::asio;

void parse_response(const std::string& raw) {
  http::response_parser<http::string_body> parser;
  parser.eager(true);  // read body in put()
  beast::error_code ec;
  parser.put(net::const_buffer(raw.data(), raw.size()), ec);
  // parser.is_done(), parser.get().body()
}
```

**Body types you will use:**

| Type | When |
|------|------|
| `string_body` | Small JSON / text |
| `empty_body` | HEAD / no payload |
| `file_body` | Static files |
| `buffer_body` | Streaming / custom |

## HTTP — sync client sketch (blocking)

Good for tools/scripts; production usually uses async.

```cpp
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <string>

namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;
using tcp = net::ip::tcp;

std::string http_get(const std::string& host, const std::string& port,
                     const std::string& target) {
  net::io_context ioc;
  tcp::resolver resolver(ioc);
  beast::tcp_stream stream(ioc);

  auto const results = resolver.resolve(host, port);
  stream.connect(results);

  http::request<http::string_body> req{http::verb::get, target, 11};
  req.set(http::field::host, host);
  req.set(http::field::user_agent, "lib_smoke");
  http::write(stream, req);

  beast::flat_buffer buffer;
  http::response<http::string_body> res;
  http::read(stream, buffer, res);

  beast::error_code ec;
  stream.socket().shutdown(tcp::socket::shutdown_both, ec);
  return res.body();
}
```

## HTTP — async server skeleton

```cpp
// One connection: read request → write response → close
// Run many sessions; each uses tcp::socket + flat_buffer.
// Always: http::async_read → handle → http::async_write
// Use strand if sharing state across handlers.
```

Key free functions:

| Sync | Async |
|------|-------|
| `http::write` | `http::async_write` |
| `http::read` | `http::async_read` |

## WebSocket — core flow

```text
1. TCP connect
2. websocket::stream<tcp_stream> ws{ioc};
3. ws.handshake(host, "/path");     // HTTP upgrade
4. ws.write(net::buffer(msg));
5. ws.read(buffer);
6. ws.close(websocket::close_code::normal);
```

### Sync WebSocket client sketch

```cpp
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <string>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

void ws_echo_once(const std::string& host, const std::string& port) {
  net::io_context ioc;
  tcp::resolver resolver(ioc);
  websocket::stream<beast::tcp_stream> ws(ioc);

  auto results = resolver.resolve(host, port);
  beast::get_lowest_layer(ws).connect(results);
  ws.handshake(host, "/");

  ws.write(net::buffer(std::string("ping")));
  beast::flat_buffer buffer;
  ws.read(buffer);
  // beast::buffers_to_string(buffer.data())

  ws.close(websocket::close_code::normal);
}
```

### Production tips for feeds

| Topic | Practice |
|-------|----------|
| **Binary vs text** | `ws.binary(true)` for protobuf/binary frames |
| **Backpressure** | Don’t `write` unbounded; queue + one in-flight write |
| **Ping/pong** | Beast handles control frames; set timeouts on `tcp_stream` |
| **TLS** | `websocket::stream<ssl_stream<tcp_stream>>` |
| **Reconnect** | State machine outside Beast; exponential backoff |
| **CPU work** | Parse (simdjson) **off** the io thread — Taskflow/TBB/pool |

## SSL (HTTPS / WSS) outline

```text
net::ssl::context ctx{net::ssl::context::tlsv12_client};
// load certs / set_default_verify_paths()
beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
// TCP connect → ssl handshake → http::write/read
// For WS: websocket::stream<beast::ssl_stream<beast::tcp_stream>>
```

Link OpenSSL (Homebrew `openssl@3`); Asio SSL headers required.

## How Beast fits your stack

```text
Beast WS/HTTP  →  bytes
      ↓
  simdjson / protobuf parse   (CPU — Taskflow or TBB)
      ↓
  domain logic / Eigen
      ↓
  gRPC outbound (optional service boundary)
```

## Project commands

```bash
make build
./build/test_beast_asio
cd build && ctest -R beast --output-on-failure
```

## Pitfalls

1. **Using standalone `asio::` types with Beast** — Beast wants `boost::asio`.
2. **Forgetting `prepare_payload()`** — wrong Content-Length.
3. **Parser not `eager(true)`** when expecting body via `put()`.
4. **Blocking parse on io_context threads** — kills latency under load.
5. **One `websocket::stream` from multiple threads** without a strand.

## Further reading

- [Boost.Beast](https://www.boost.org/doc/libs/release/libs/beast/)
- [Beast examples](https://github.com/boostorg/beast) (http client/server, websocket)
- Project: `tests/test_beast_asio.cpp`, [asio.md](./asio.md)
