# simdjson

**What it is:** Extremely fast JSON parser (SIMD) with on-demand and DOM APIs.  
**Install:** Homebrew `simdjson`  
**Smoke:** `tests/test_libs.cpp` tag `[json]` / `[simdjson]`, main smoke “nlohmann-json + simdjson”  
**Companion:** `nlohmann/json` for ergonomic DOM when speed is not critical

## Why simdjson?

| Strength | Detail |
|----------|--------|
| **Speed** | Often 2–4+ GB/s class throughput on modern CPUs |
| **On-demand** | Walk only fields you need; less allocation |
| **Validation** | Full JSON validation while parsing |
| **Production** | Used in high-throughput services and data paths |

Use **nlohmann** for config files and small APIs. Use **simdjson** on **hot paths** (market data, large payloads).

## Two APIs

| API | Header / types | When |
|-----|----------------|------|
| **On-demand** | `simdjson::ondemand::parser` | Hot path, stream-like field access |
| **DOM** | `simdjson::dom::parser` | Random access, longer-lived document |

This project’s smoke uses **on-demand**.

## Critical rule: padded input

simdjson needs **SIMD padding** after the JSON bytes (capacity ≥ length + `SIMDJSON_PADDING`).

```cpp
// WRONG — temporary padded_string rvalue often deleted API
// parser.iterate(simdjson::padded_string{body});

// RIGHT — keep padded_string alive for the document lifetime
simdjson::padded_string pad{body};
auto doc = parser.iterate(pad);
```

From `tests/test_libs.cpp`.

## Minimal examples

### 1. On-demand field access

```cpp
#include <simdjson.h>
#include <string>
#include <iostream>

int main() {
  const std::string body = R"({"x":10,"y":[1,2,3]})";
  simdjson::ondemand::parser parser;
  simdjson::padded_string pad{body};
  auto doc = parser.iterate(pad);

  int64_t x = doc["x"];
  std::cout << x << "\n";  // 10

  simdjson::ondemand::array arr = doc["y"];
  int sum = 0;
  for (auto v : arr) {
    int64_t n = v;
    sum += static_cast<int>(n);
  }
  // sum == 6
}
```

### 2. Nested object (tick-like)

```cpp
#include <simdjson.h>
#include <string>

struct Tick {
  std::string_view symbol;
  double px;
  double qty;
};

Tick parse_tick(std::string_view json) {
  thread_local simdjson::ondemand::parser parser;
  simdjson::padded_string pad{json};
  auto doc = parser.iterate(pad);

  Tick t;
  t.symbol = doc["symbol"];   // string_view into pad — careful with lifetime
  t.px = doc["px"];
  t.qty = doc["qty"];
  return t;  // if you need owned strings, copy symbol to std::string
}
```

**Lifetime:** on-demand `string_view` points into the padded buffer. Copy if the buffer dies.

### 3. Error handling

```cpp
#include <simdjson.h>

void safe_parse(const simdjson::padded_string& pad) {
  simdjson::ondemand::parser parser;
  simdjson::ondemand::document doc;
  auto error = parser.iterate(pad).get(doc);
  if (error) {
    // error == simdjson::error_code
    return;
  }
  double px;
  if (doc["px"].get(px) != simdjson::SUCCESS) {
    return;
  }
}
```

Prefer `.get(var)` in production; exceptions exist but explicit codes are clearer on hot paths.

### 4. Reuse the parser

```cpp
// One parser per thread (not shared across threads)
thread_local simdjson::ondemand::parser parser;
```

Allocation-heavy if you construct a new parser every message.

### 5. nlohmann when ergonomics win

```cpp
#include <nlohmann/json.hpp>

void config() {
  nlohmann::json j = {{"threads", 4}, {"symbol", "ES"}};
  int n = j.at("threads");
  // dump, patch, schema-ish helpers — slower, nicer
}
```

## Performance practices

| Do | Don’t |
|----|-------|
| Reuse `parser` per thread | Share one parser across threads |
| Keep `padded_string` alive | Use dangling `string_view` from doc |
| Parse only needed fields | Materialize entire DOM for one field |
| Batch / arena your outputs | Allocate `std::string` per field blindly |
| Validate error codes | Ignore `error_code` on untrusted input |

## Fit with Asio / Beast / Taskflow

```text
Beast WS message (bytes)
    → copy/append into padded buffer (or pad in place if capacity allows)
    → simdjson ondemand on worker (Taskflow/TBB), not on io_context thread
    → emit domain structs to next stage
```

## Project commands

```bash
make build
cd build && ctest -R 'simdjson|json' --output-on-failure
./build/lib_smoke
```

CMake: `find_package(simdjson CONFIG REQUIRED)` → `simdjson::simdjson`

## Pitfalls

1. **Temporary `padded_string`** — `iterate(padded_string{...})` is often deleted; keep an lvalue.
2. **Lifetime of views** — document views die with the pad.
3. **NDJSON / multi-value** — use the multi-document APIs carefully; not the same as one object.
4. **Huge numbers / decimals** — know int64 vs double; financial code may want integer ticks.
5. **UTF-8** — invalid UTF-8 fails validation (good for security).

## Further reading

- [simdjson.org](https://simdjson.org/)
- [On-demand API](https://github.com/simdjson/simdjson/blob/master/doc/ondemand.md)
- Project: `tests/test_libs.cpp`, `src/main.cpp` (`smoke_json`)
