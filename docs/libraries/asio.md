# Asio (standalone + Boost.Asio) and Beast

**What it is:** Asynchronous I/O framework — timers, sockets, strands, executors.  
**In this project:** Homebrew `asio` (standalone) + Boost.Beast/Boost.Asio.  
**Smoke:** `tests/test_beast_asio.cpp`, `make` / `make run`

## Two flavors you will see

| Flavor | Header | Notes |
|--------|--------|-------|
| **Standalone Asio** | `#include <asio.hpp>` | No Boost dependency; define `ASIO_STANDALONE` |
| **Boost.Asio** | `#include <boost/asio.hpp>` | Ships with Boost; Beast uses this |

This project’s CMake defines `ASIO_STANDALONE` for the standalone include path.
Beast still uses `boost::asio` / `boost::beast` namespaces.

## Mental model

1. **`io_context`** — the event loop (queue of work).
2. **Handlers** — functions run when async ops complete.
3. **`run()`** — blocks (or runs) until work is done / stopped.
4. **Executors** — *where* work runs (thread pool, strand for serialisation).

```text
  post/async_op ──► io_context queue ──► run() dispatches handlers
```

## Minimal examples

### 1. Post work (standalone)

```cpp
#include <asio.hpp>
#include <atomic>
#include <iostream>

int main() {
  asio::io_context ioc;
  std::atomic<int> n{0};

  asio::post(ioc, [&] { n.fetch_add(1); });
  asio::post(ioc, [&] { n.fetch_add(1); });

  ioc.run();  // runs both handlers
  std::cout << n.load() << "\n";  // 2
}
```

### 2. Timer (Boost.Asio)

```cpp
#include <boost/asio.hpp>
#include <chrono>
#include <iostream>

namespace net = boost::asio;

int main() {
  net::io_context ioc;
  net::steady_timer timer(ioc, std::chrono::milliseconds(50));

  timer.async_wait([](const boost::system::error_code& ec) {
    if (!ec) std::cout << "tick\n";
  });

  ioc.run();
}
```

### 3. Beast: build an HTTP request (no network)

```cpp
#include <boost/beast/http.hpp>
#include <iostream>

namespace http = boost::beast::http;

int main() {
  http::request<http::string_body> req{http::verb::get, "/health", 11};
  req.set(http::field::host, "localhost");
  req.set(http::field::user_agent, "lib_smoke");
  req.prepare_payload();

  std::cout << req.method_string() << " " << req.target() << "\n";
}
```

### 4. Beast: parse an HTTP response buffer

```cpp
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/buffer.hpp>
#include <string>

namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;

void parse() {
  const std::string raw =
      "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello";

  http::response_parser<http::string_body> parser;
  parser.eager(true);
  beast::error_code ec;
  parser.put(net::const_buffer(raw.data(), raw.size()), ec);
  // parser.is_done() && parser.get().body() == "hello"
}
```

## Patterns you should learn next

| Pattern | Purpose |
|---------|---------|
| **`strand`** | Serialise handlers that touch shared state |
| **Thread pool + `io_context`** | Multi-threaded server (`run()` on N threads) |
| **Cancellation** | `cancellation_signal` / slots (modern Asio) |
| **Coroutines** | `asio::awaitable` + `co_spawn` (C++20) |
| **Beast WebSocket** | Streaming feeds over WS |

## How this fits your other libs

- **Taskflow / TBB:** CPU-heavy stages *after* Asio receives data (don’t block `io_context` threads).
- **stdexec:** Different composition model; can bridge with executors carefully.
- **Folly Futures:** Overlapping async style; pick one primary model per subsystem.
- **HPX:** Not a network stack; use for parallel compute, not as Asio replacement.

## Project commands

```bash
make configure && make build
./build/test_beast_asio          # Catch2 Asio/Beast tests
# or
cd build && ctest -R 'Asio|Beast' --output-on-failure
```

## Common pitfalls

1. **Forgetting `run()`** — posted work never executes.
2. **Blocking inside a handler** — starves the io_context; offload to a worker pool.
3. **Lifetime-free shared state** without a strand/mutex.
4. **Mixing standalone and Boost.Asio types** in the same call — don’t.
5. **HPX bundling Asio** — when `LIB_SMOKE_WITH_HPX=ON`, prefer Homebrew Asio includes for standalone smoke (Makefile/CMake already hint Homebrew first).

## Further reading

- [Asio docs](https://think-async.com/Asio/)
- **HTTP/WebSocket deep-dive:** [beast.md](./beast.md)
- Project tests: `tests/test_beast_asio.cpp`
