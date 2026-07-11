cmake_minimum_required(VERSION 3.28)

project(cpp26_systems_stack
  VERSION 1.0.0
  DESCRIPTION "C++26 systems stack — async I/O, parallelism, JSON, RPC integration"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# Optional heavy libraries (default OFF so base configure stays fast)
# ---------------------------------------------------------------------------
option(LIB_SMOKE_WITH_FOLLY "Enable Folly smoke checks (Homebrew: brew install folly)" OFF)
option(LIB_SMOKE_WITH_HPX   "Enable HPX smoke checks (local install, not Homebrew)" OFF)

# Optional search hints (HPX is not on Homebrew)
set(LIB_SMOKE_HPX_ROOT "$ENV{HOME}/cpp-deps/hpx" CACHE PATH
  "Install prefix for HPX (default: ~/cpp-deps/hpx)")

# Homebrew prefix (Apple Silicon default; overridable)
if(NOT DEFINED HOMEBREW_PREFIX)
  if(EXISTS /opt/homebrew)
    set(HOMEBREW_PREFIX /opt/homebrew)
  elseif(EXISTS /usr/local/Homebrew)
    set(HOMEBREW_PREFIX /usr/local)
  else()
    set(HOMEBREW_PREFIX /opt/homebrew)
  endif()
endif()
list(PREPEND CMAKE_PREFIX_PATH "${HOMEBREW_PREFIX}")
if(LIB_SMOKE_WITH_HPX AND EXISTS "${LIB_SMOKE_HPX_ROOT}")
  list(PREPEND CMAKE_PREFIX_PATH "${LIB_SMOKE_HPX_ROOT}")
endif()

include(FetchContent)
include(CTest)
enable_testing()

# ---------------------------------------------------------------------------
# System / Homebrew packages
# ---------------------------------------------------------------------------
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(Catch2 3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(simdjson CONFIG REQUIRED)
find_package(range-v3 CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED)

# Header-only: standalone Asio + Taskflow (prefer Homebrew over HPX-bundled Asio)
find_path(ASIO_INCLUDE_DIR asio.hpp
  HINTS "${HOMEBREW_PREFIX}/opt/asio/include" "${HOMEBREW_PREFIX}/include"
  NO_DEFAULT_PATH
  REQUIRED
)
find_path(TASKFLOW_INCLUDE_DIR taskflow/taskflow.hpp
  HINTS "${HOMEBREW_PREFIX}/opt/taskflow/include" "${HOMEBREW_PREFIX}/include"
  NO_DEFAULT_PATH
  REQUIRED
)

# ---------------------------------------------------------------------------
# Optional: Folly (Homebrew)
# ---------------------------------------------------------------------------
if(LIB_SMOKE_WITH_FOLLY)
  find_package(folly CONFIG REQUIRED)
  # Homebrew Folly::folly_deps injects the macOS CommandLineTools C SDK
  # include dir, which reintroduces math.h macros (isnan/isfinite) and
  # breaks libc++ <complex> + fmt when mixed with Beast/Asio TUs.
  if(TARGET Folly::folly_deps)
    get_target_property(_folly_dep_incs Folly::folly_deps INTERFACE_INCLUDE_DIRECTORIES)
    if(_folly_dep_incs)
      list(FILTER _folly_dep_incs EXCLUDE REGEX "CommandLineTools|.sdk/usr/include")
      set_property(TARGET Folly::folly_deps PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${_folly_dep_incs}")
    endif()
  endif()
  message(STATUS "lib_smoke: Folly enabled (${FOLLY_PREFIX_DIR})")
endif()

# ---------------------------------------------------------------------------
# Optional: HPX (local install — not packaged in Homebrew)
# ---------------------------------------------------------------------------
if(LIB_SMOKE_WITH_HPX)
  find_package(HPX CONFIG REQUIRED)
  message(STATUS "lib_smoke: HPX enabled (${HPX_DIR})")
endif()

# ---------------------------------------------------------------------------
# stdexec (P2300) — not in Homebrew; fetch per-project
# ---------------------------------------------------------------------------
set(STDEXEC_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(STDEXEC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  stdexec
  GIT_REPOSITORY https://github.com/NVIDIA/stdexec.git
  GIT_TAG        nvhpc-26.05
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(stdexec)

# ---------------------------------------------------------------------------
# Shared compile options / interface for all smoke targets
# ---------------------------------------------------------------------------
add_library(smoke_deps INTERFACE)
target_include_directories(smoke_deps INTERFACE
  "${ASIO_INCLUDE_DIR}"
  "${TASKFLOW_INCLUDE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_compile_definitions(smoke_deps INTERFACE
  ASIO_STANDALONE
  ASIO_NO_DEPRECATED
)
target_link_libraries(smoke_deps INTERFACE
  fmt::fmt
  spdlog::spdlog
  TBB::tbb
  protobuf::libprotobuf
  gRPC::grpc++
  nlohmann_json::nlohmann_json
  simdjson::simdjson
  range-v3::range-v3
  Eigen3::Eigen
  absl::strings
  absl::str_format
  Boost::headers
  STDEXEC::stdexec
)

# Keep Folly/HPX off the shared interface: Folly's include/flags can break
# Beast/Asio TUs (e.g. isnan macro clashes with libc++ <complex>).
if(LIB_SMOKE_WITH_FOLLY)
  target_compile_definitions(smoke_deps INTERFACE LIB_SMOKE_HAS_FOLLY=1)
  add_library(smoke_folly INTERFACE)
  target_link_libraries(smoke_folly INTERFACE Folly::folly)
else()
  target_compile_definitions(smoke_deps INTERFACE LIB_SMOKE_HAS_FOLLY=0)
endif()

if(LIB_SMOKE_WITH_HPX)
  target_compile_definitions(smoke_deps INTERFACE LIB_SMOKE_HAS_HPX=1)
  add_library(smoke_hpx INTERFACE)
  target_link_libraries(smoke_hpx INTERFACE HPX::hpx)
else()
  target_compile_definitions(smoke_deps INTERFACE LIB_SMOKE_HAS_HPX=0)
endif()

find_package(Threads REQUIRED)
target_link_libraries(smoke_deps INTERFACE Threads::Threads)

# ---------------------------------------------------------------------------
# Low-latency portable primitives (header-only, under include/ll/)
# ---------------------------------------------------------------------------
add_library(ll_headers INTERFACE)
target_include_directories(ll_headers INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(ll_headers INTERFACE cxx_std_26)
target_link_libraries(ll_headers INTERFACE Threads::Threads)

# ---------------------------------------------------------------------------
# Protobuf smoke message
# ---------------------------------------------------------------------------
include("${Protobuf_DIR}/protobuf-generate.cmake")
add_library(smoke_proto STATIC proto/smoke.proto)
target_link_libraries(smoke_proto PUBLIC protobuf::libprotobuf)
target_include_directories(smoke_proto PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_features(smoke_proto PUBLIC cxx_std_26)
protobuf_generate(
  TARGET smoke_proto
  LANGUAGE cpp
  PROTOS "${CMAKE_CURRENT_SOURCE_DIR}/proto/smoke.proto"
  PROTOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}"
  IMPORT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/proto"
)

# ---------------------------------------------------------------------------
# Main smoke binary — exercises every library once and prints status
# ---------------------------------------------------------------------------
add_executable(lib_smoke src/main.cpp)
target_link_libraries(lib_smoke PRIVATE smoke_deps smoke_proto)
if(LIB_SMOKE_WITH_FOLLY)
  target_link_libraries(lib_smoke PRIVATE smoke_folly)
endif()
if(LIB_SMOKE_WITH_HPX)
  target_link_libraries(lib_smoke PRIVATE smoke_hpx)
  # HPX apps need the HPX entry-point machinery on some platforms.
  hpx_setup_target(lib_smoke)
endif()

# ---------------------------------------------------------------------------
# Catch2 smoke tests
# ---------------------------------------------------------------------------
include(Catch)

add_executable(test_beast_asio tests/test_beast_asio.cpp)
target_link_libraries(test_beast_asio PRIVATE smoke_deps Catch2::Catch2WithMain)
catch_discover_tests(test_beast_asio)

add_executable(test_taskflow tests/test_taskflow.cpp)
target_link_libraries(test_taskflow PRIVATE smoke_deps Catch2::Catch2WithMain)
catch_discover_tests(test_taskflow)

add_executable(test_libs tests/test_libs.cpp)
target_link_libraries(test_libs PRIVATE smoke_deps smoke_proto Catch2::Catch2WithMain)
catch_discover_tests(test_libs)

add_executable(test_ll_modules tests/test_ll_modules.cpp)
target_link_libraries(test_ll_modules PRIVATE ll_headers Catch2::Catch2WithMain)
catch_discover_tests(test_ll_modules)

if(LIB_SMOKE_WITH_FOLLY)
  add_executable(test_folly tests/test_folly.cpp)
  target_link_libraries(test_folly PRIVATE smoke_deps smoke_folly Catch2::Catch2WithMain)
  catch_discover_tests(test_folly)
endif()

if(LIB_SMOKE_WITH_HPX)
  add_executable(test_hpx tests/test_hpx.cpp)
  target_link_libraries(test_hpx PRIVATE smoke_deps smoke_hpx Catch2::Catch2WithMain)
  hpx_setup_target(test_hpx)
  catch_discover_tests(test_hpx)
endif()

# ---------------------------------------------------------------------------
# Optional GTest target (verifies googletest install)
# ---------------------------------------------------------------------------
add_executable(test_gtest_smoke tests/test_gtest_smoke.cpp)
target_link_libraries(test_gtest_smoke PRIVATE smoke_deps GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(test_gtest_smoke)

# ---------------------------------------------------------------------------
# Low-latency blueprint examples (run via: make examples)
# ---------------------------------------------------------------------------
add_executable(example_spsc examples/spsc/main.cpp)
target_link_libraries(example_spsc PRIVATE ll_headers)

add_executable(example_arena examples/arena/main.cpp)
target_link_libraries(example_arena PRIVATE ll_headers)

add_executable(example_memory_order examples/memory_order/main.cpp)
target_link_libraries(example_memory_order PRIVATE ll_headers)

add_executable(example_tsc examples/tsc/main.cpp)
target_link_libraries(example_tsc PRIVATE ll_headers)

message(STATUS "lib_smoke: Homebrew prefix = ${HOMEBREW_PREFIX}")
message(STATUS "lib_smoke: Asio include    = ${ASIO_INCLUDE_DIR}")
message(STATUS "lib_smoke: Taskflow include= ${TASKFLOW_INCLUDE_DIR}")
message(STATUS "lib_smoke: C++ standard    = ${CMAKE_CXX_STANDARD}")
message(STATUS "lib_smoke: WITH_FOLLY      = ${LIB_SMOKE_WITH_FOLLY}")
message(STATUS "lib_smoke: WITH_HPX        = ${LIB_SMOKE_WITH_HPX}")
message(STATUS "ll_headers: include/ll/*   = ON (SPSC, arena, TSC, affinity)")
if(LIB_SMOKE_WITH_HPX)
  message(STATUS "lib_smoke: HPX root        = ${LIB_SMOKE_HPX_ROOT}")
endif()
