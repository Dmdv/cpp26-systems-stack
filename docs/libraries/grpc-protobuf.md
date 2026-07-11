# Protocol Buffers + gRPC

**What it is:**  
- **protobuf** — language-neutral serialization (schemas → generated code)  
- **gRPC** — RPC framework over HTTP/2 using protobuf by default  

**Install:** Homebrew `protobuf`, `grpc`  
**Smoke:** `proto/smoke.proto`, generated `smoke.pb.h`, tests in `test_libs` / main  
**CMake:** `protobuf_generate` + `protobuf::libprotobuf` + `gRPC::grpc++`

## Why protobuf + gRPC?

| Need | Tool |
|------|------|
| Stable binary/json schema across languages | **protobuf** |
| Versioned service APIs (request/response streams) | **gRPC** |
| Faster / stricter than free-form JSON for service boundaries | both |

For **in-process** hot path JSON, prefer **simdjson**. For **cross-service** contracts, prefer **protobuf**.

## Mental model

```text
  .proto schema
       │  protoc / protobuf_generate
       ▼
  generated *.pb.h / *.pb.cc   (+ *.grpc.pb.* for services)
       │
       ├─► SerializeToString / ParseFromString   (messages only)
       └─► Stub / Service                        (gRPC RPCs)
```

## Project smoke schema

`proto/smoke.proto`:

```protobuf
syntax = "proto3";

package smoke;

message SmokePing {
  string name = 1;
  int32  count = 2;
  repeated string tags = 3;
}
```

### C++ usage (messages only)

```cpp
#include "smoke.pb.h"
#include <string>

void roundtrip() {
  smoke::SmokePing p;
  p.set_name("catch2");
  p.set_count(1);
  p.add_tags("test");

  std::string out;
  p.SerializeToString(&out);

  smoke::SmokePing q;
  q.ParseFromString(out);
  // q.name() == "catch2"
}
```

From `tests/test_libs.cpp` / `src/main.cpp`.

## CMake integration (this project)

```cmake
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

add_library(smoke_proto STATIC proto/smoke.proto)
target_link_libraries(smoke_proto PUBLIC protobuf::libprotobuf)
target_include_directories(smoke_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate(
  TARGET smoke_proto
  LANGUAGE cpp
  PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/proto/smoke.proto
  PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}
  IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/proto
)
```

Link app targets with `smoke_proto` and, for RPC, `gRPC::grpc++`.

Smoke only **links** gRPC and prints `grpc::Version()` — no server.

## Defining a service (next step beyond smoke)

```protobuf
syntax = "proto3";
package market;

message QuoteRequest  { string symbol = 1; }
message QuoteResponse { string symbol = 1; double bid = 2; double ask = 3; }

service QuoteService {
  rpc GetQuote(QuoteRequest) returns (QuoteResponse);
  rpc StreamQuotes(QuoteRequest) returns (stream QuoteResponse);
}
```

Generate with `LANGUAGE grpc` / plugin `grpc_cpp_plugin` (gRPC CMake helpers or `protobuf_generate` with `PLUGIN`).

### Server sketch

```cpp
// class QuoteServiceImpl final : public market::QuoteService::Service {
//   Status GetQuote(ServerContext*, const QuoteRequest* req,
//                   QuoteResponse* res) override {
//     res->set_symbol(req->symbol());
//     res->set_bid(100.0);
//     res->set_ask(100.25);
//     return Status::OK;
//   }
// };
//
// ServerBuilder builder;
// builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
// builder.RegisterService(&service);
// auto server = builder.BuildAndStart();
// server->Wait();
```

### Client sketch

```cpp
// auto channel = grpc::CreateChannel("localhost:50051",
//                    grpc::InsecureChannelCredentials());
// auto stub = market::QuoteService::NewStub(channel);
// QuoteRequest req; req.set_symbol("ES");
// QuoteResponse res;
// ClientContext ctx;
// Status st = stub->GetQuote(&ctx, req, &res);
```

Use TLS credentials in real deployments (`SslServerCredentials` / channel args).

## RPC styles

| Pattern | Proto | Use |
|---------|-------|-----|
| Unary | `rpc M(Req) returns (Res)` | Simple query |
| Server stream | `returns (stream Res)` | Market data fan-out |
| Client stream | `rpc M(stream Req) returns (Res)` | Bulk upload |
| Bidi stream | `stream` both | Interactive / multiplexed |

## JSON vs protobuf on the wire

| | JSON (simdjson) | protobuf |
|--|-----------------|----------|
| Schema | Informal | `.proto` required |
| Size / speed | Larger text | Compact binary |
| Browser | Natural | Needs gRPC-Web / REST gateway |
| Evolution | Ad-hoc | Field numbers, reserved |

Many systems: **JSON at edge**, **protobuf internally**.

## Fit with your stack

```text
External WS (Beast) JSON ──simdjson──► internal structs
                                      │
                                      ▼
                              optional protobuf encode
                                      │
                                      ▼
                              gRPC to risk/pricing service
```

Or pure gRPC microservices without Beast.

## Project commands

```bash
make build
cd build && ctest -R 'protobuf|gRPC' --output-on-failure
./build/lib_smoke   # serialize/parse + grpc version
ls build/smoke.pb.h build/smoke.pb.cc
```

## Pitfalls

1. **Field numbers are identity** — never reuse numbers; use `reserved`.
2. **proto2 vs proto3** — stick to proto3 defaults (no presence for scalars unless `optional`).
3. **Generated code in build dir** — add binary dir to includes (project does).
4. **Blocking gRPC on Asio threads** — use separate pools or async gRPC APIs.
5. **Large messages** — set max send/receive size; prefer streaming.
6. **ABI / version skew** — rebuild all services on schema change; CI proto checks help.

## Security checklist (services)

- Prefer TLS; avoid `Insecure*` outside local dev  
- Auth: tokens/mTLS interceptors  
- Deadlines on every RPC (`ClientContext::set_deadline`)  
- Validate untrusted fields after parse  

## Further reading

- [Protobuf C++ tutorial](https://protobuf.dev/getting-started/cpptutorial/)
- [gRPC C++ quickstart](https://grpc.io/docs/languages/cpp/quickstart/)
- Project: `proto/smoke.proto`, `tests/test_libs.cpp`, `CMakeLists.txt` (`smoke_proto`)
