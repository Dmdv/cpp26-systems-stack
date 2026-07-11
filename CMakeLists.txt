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

# Industry low-latency libraries (auto-enable when found unless forced OFF)
option(STACK_WITH_HWLOC        "Use hwloc for topology / pinning helpers" ON)
option(STACK_WITH_FLATBUFFERS  "Enable FlatBuffers zero-copy serialization" ON)
option(STACK_WITH_BENCHMARK    "Build Google Benchmark microbenchmarks" ON)
option(STACK_WITH_MIMALLOC     "Link mimalloc for off-path allocations" ON)
option(STACK_WITH_MOODYCAMEL   "Fetch moodycamel ReaderWriterQueue" ON)
option(STACK_WITH_LIBNUMA      "Linux: libnuma (optional)" ON)
option(STACK_WITH_LIBURING     "Linux: liburing / io_uring (optional)" ON)

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

# LL_HAS_* macros default to 0 in include/ll/industry.hpp; CMake sets =1 when found.

# ---------------------------------------------------------------------------
# Industry libraries — auto-detect / FetchContent (see docs/tutorials/)
# ---------------------------------------------------------------------------

# moodycamel::ReaderWriterQueue (header-only SPSC benchmark standard)
set(STACK_HAS_MOODYCAMEL 0)
if(STACK_WITH_MOODYCAMEL)
  FetchContent_Declare(
    readerwriterqueue
    GIT_REPOSITORY https://github.com/cameron314/readerwriterqueue.git
    GIT_TAG        v1.0.6
    GIT_SHALLOW    TRUE
  )
  FetchContent_GetProperties(readerwriterqueue)
  if(NOT readerwriterqueue_POPULATED)
    FetchContent_Populate(readerwriterqueue)
  endif()
  add_library(moodycamel_rwq INTERFACE)
  target_include_directories(moodycamel_rwq INTERFACE "${readerwriterqueue_SOURCE_DIR}")
  target_compile_definitions(ll_headers INTERFACE LL_HAS_MOODYCAMEL=1)
  set(STACK_HAS_MOODYCAMEL 1)
  message(STATUS "stack: moodycamel ReaderWriterQueue  = ON (FetchContent)")
endif()

# hwloc — portable topology (Open MPI)
set(STACK_HAS_HWLOC 0)
if(STACK_WITH_HWLOC)
  find_path(HWLOC_INCLUDE_DIR hwloc.h
    HINTS "${HOMEBREW_PREFIX}/include" "${HOMEBREW_PREFIX}/opt/hwloc/include")
  find_library(HWLOC_LIBRARY hwloc
    HINTS "${HOMEBREW_PREFIX}/lib" "${HOMEBREW_PREFIX}/opt/hwloc/lib")
  if(HWLOC_INCLUDE_DIR AND HWLOC_LIBRARY)
    add_library(stack_hwloc INTERFACE)
    target_include_directories(stack_hwloc INTERFACE "${HWLOC_INCLUDE_DIR}")
    target_link_libraries(stack_hwloc INTERFACE "${HWLOC_LIBRARY}")
    target_compile_definitions(ll_headers INTERFACE LL_HAS_HWLOC=1)
    set(STACK_HAS_HWLOC 1)
    message(STATUS "stack: hwloc                       = ON (${HWLOC_LIBRARY})")
  else()
    message(STATUS "stack: hwloc                       = OFF (not found)")
  endif()
endif()

# FlatBuffers — zero-copy tables
set(STACK_HAS_FLATBUFFERS 0)
if(STACK_WITH_FLATBUFFERS)
  find_package(flatbuffers CONFIG QUIET)
  if(TARGET flatbuffers::flatbuffers)
    find_program(FLATC_EXECUTABLE flatc
      HINTS "${HOMEBREW_PREFIX}/bin" "${HOMEBREW_PREFIX}/opt/flatbuffers/bin")
    if(NOT FLATC_EXECUTABLE AND TARGET flatbuffers::flatc)
      get_target_property(FLATC_EXECUTABLE flatbuffers::flatc IMPORTED_LOCATION_RELEASE)
      if(NOT FLATC_EXECUTABLE)
        get_target_property(FLATC_EXECUTABLE flatbuffers::flatc IMPORTED_LOCATION)
      endif()
    endif()
    if(FLATC_EXECUTABLE)
      set(FB_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
      file(MAKE_DIRECTORY "${FB_GEN_DIR}")
      set(FB_SCHEMA "${CMAKE_CURRENT_SOURCE_DIR}/schemas/tick.fbs")
      # flatc emits tick_generated.h from schemas/tick.fbs
      set(STACK_FB_GEN_HEADER "${FB_GEN_DIR}/tick_generated.h")
      add_custom_command(
        OUTPUT "${STACK_FB_GEN_HEADER}"
        COMMAND "${FLATC_EXECUTABLE}" --cpp -o "${FB_GEN_DIR}" "${FB_SCHEMA}"
        DEPENDS "${FB_SCHEMA}"
        COMMENT "flatc: tick.fbs -> tick_generated.h"
        VERBATIM
      )
      add_library(stack_flatbuffers INTERFACE)
      target_link_libraries(stack_flatbuffers INTERFACE flatbuffers::flatbuffers)
      target_include_directories(stack_flatbuffers INTERFACE "${FB_GEN_DIR}")
      target_compile_definitions(ll_headers INTERFACE LL_HAS_FLATBUFFERS=1)
      set(STACK_HAS_FLATBUFFERS 1)
      message(STATUS "stack: FlatBuffers                 = ON (flatc=${FLATC_EXECUTABLE})")
    else()
      message(STATUS "stack: FlatBuffers                 = OFF (flatc not found)")
    endif()
  else()
    message(STATUS "stack: FlatBuffers                 = OFF (package not found)")
  endif()
endif()

# Google Benchmark
set(STACK_HAS_BENCHMARK 0)
if(STACK_WITH_BENCHMARK)
  find_package(benchmark CONFIG QUIET)
  if(TARGET benchmark::benchmark)
    set(STACK_HAS_BENCHMARK 1)
    target_compile_definitions(ll_headers INTERFACE LL_HAS_BENCHMARK=1)
    message(STATUS "stack: Google Benchmark            = ON")
  else()
    message(STATUS "stack: Google Benchmark            = OFF (not found)")
  endif()
endif()

# mimalloc (off critical path)
set(STACK_HAS_MIMALLOC 0)
if(STACK_WITH_MIMALLOC)
  find_package(mimalloc CONFIG QUIET)
  if(TARGET mimalloc AND NOT TARGET mimalloc::mimalloc)
    add_library(mimalloc::mimalloc ALIAS mimalloc)
  endif()
  if(TARGET mimalloc::mimalloc OR TARGET mimalloc)
    if(NOT TARGET stack_mimalloc)
      add_library(stack_mimalloc INTERFACE)
      if(TARGET mimalloc::mimalloc)
        target_link_libraries(stack_mimalloc INTERFACE mimalloc::mimalloc)
      else()
        target_link_libraries(stack_mimalloc INTERFACE mimalloc)
      endif()
    endif()
    target_compile_definitions(ll_headers INTERFACE LL_HAS_MIMALLOC=1)
    set(STACK_HAS_MIMALLOC 1)
    message(STATUS "stack: mimalloc                    = ON")
  else()
    # Fallback: bare library
    find_library(MIMALLOC_LIBRARY mimalloc
      HINTS "${HOMEBREW_PREFIX}/lib" "${HOMEBREW_PREFIX}/opt/mimalloc/lib")
    find_path(MIMALLOC_INCLUDE_DIR mimalloc.h
      HINTS "${HOMEBREW_PREFIX}/include" "${HOMEBREW_PREFIX}/opt/mimalloc/include")
    if(MIMALLOC_LIBRARY AND MIMALLOC_INCLUDE_DIR)
      add_library(stack_mimalloc INTERFACE)
      target_include_directories(stack_mimalloc INTERFACE "${MIMALLOC_INCLUDE_DIR}")
      target_link_libraries(stack_mimalloc INTERFACE "${MIMALLOC_LIBRARY}")
      target_compile_definitions(ll_headers INTERFACE LL_HAS_MIMALLOC=1)
      set(STACK_HAS_MIMALLOC 1)
      message(STATUS "stack: mimalloc                    = ON (${MIMALLOC_LIBRARY})")
    else()
      message(STATUS "stack: mimalloc                    = OFF (not found)")
    endif()
  endif()
endif()

# Linux-only: libnuma / liburing (soft)
set(STACK_HAS_LIBNUMA 0)
set(STACK_HAS_LIBURING 0)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  if(STACK_WITH_LIBNUMA)
    find_library(NUMA_LIBRARY numa)
    find_path(NUMA_INCLUDE_DIR numa.h)
    if(NUMA_LIBRARY AND NUMA_INCLUDE_DIR)
      add_library(stack_numa INTERFACE)
      target_include_directories(stack_numa INTERFACE "${NUMA_INCLUDE_DIR}")
      target_link_libraries(stack_numa INTERFACE "${NUMA_LIBRARY}")
      target_compile_definitions(ll_headers INTERFACE LL_HAS_LIBNUMA=1)
      set(STACK_HAS_LIBNUMA 1)
      message(STATUS "stack: libnuma                     = ON")
    endif()
  endif()
  if(STACK_WITH_LIBURING)
    find_library(URING_LIBRARY uring)
    find_path(URING_INCLUDE_DIR liburing.h)
    if(URING_LIBRARY AND URING_INCLUDE_DIR)
      add_library(stack_uring INTERFACE)
      target_include_directories(stack_uring INTERFACE "${URING_INCLUDE_DIR}")
      target_link_libraries(stack_uring INTERFACE "${URING_LIBRARY}")
      target_compile_definitions(ll_headers INTERFACE LL_HAS_LIBURING=1)
      set(STACK_HAS_LIBURING 1)
      message(STATUS "stack: liburing                    = ON")
    endif()
  endif()
endif()

# Industry interface target used by tests/examples
add_library(stack_industry INTERFACE)
target_link_libraries(stack_industry INTERFACE ll_headers Boost::headers)
if(STACK_HAS_MOODYCAMEL)
  target_link_libraries(stack_industry INTERFACE moodycamel_rwq)
endif()
if(STACK_HAS_HWLOC)
  target_link_libraries(stack_industry INTERFACE stack_hwloc)
endif()
if(STACK_HAS_FLATBUFFERS)
  target_link_libraries(stack_industry INTERFACE stack_flatbuffers)
endif()
if(STACK_HAS_MIMALLOC)
  target_link_libraries(stack_industry INTERFACE stack_mimalloc)
endif()
if(STACK_HAS_LIBNUMA)
  target_link_libraries(stack_industry INTERFACE stack_numa)
endif()
if(STACK_HAS_LIBURING)
  target_link_libraries(stack_industry INTERFACE stack_uring)
endif()

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

# Industry stack: pmr, HDR hist, SBE-style, Boost.Lockfree, optional hwloc/FB/mimalloc/moodycamel
set(_industry_sources tests/test_industry_stack.cpp)
if(STACK_HAS_FLATBUFFERS)
  list(APPEND _industry_sources "${STACK_FB_GEN_HEADER}")
endif()
add_executable(test_industry_stack ${_industry_sources})
target_link_libraries(test_industry_stack PRIVATE stack_industry Catch2::Catch2WithMain)
catch_discover_tests(test_industry_stack)

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

add_executable(example_pmr examples/pmr/main.cpp)
target_link_libraries(example_pmr PRIVATE ll_headers)

add_executable(example_hdr examples/hdr/main.cpp)
target_link_libraries(example_hdr PRIVATE ll_headers)

add_executable(example_sbe_style examples/sbe_style/main.cpp)
target_link_libraries(example_sbe_style PRIVATE ll_headers)

add_executable(example_industry_queues examples/industry_queues/main.cpp)
target_link_libraries(example_industry_queues PRIVATE stack_industry)

# Google Benchmark microbenches (optional)
if(STACK_HAS_BENCHMARK)
  add_executable(bench_queues benchmarks/bench_queues.cpp)
  target_link_libraries(bench_queues PRIVATE stack_industry benchmark::benchmark)
  if(NOT WIN32)
    target_link_libraries(bench_queues PRIVATE pthread)
  endif()
endif()

message(STATUS "lib_smoke: Homebrew prefix = ${HOMEBREW_PREFIX}")
message(STATUS "lib_smoke: Asio include    = ${ASIO_INCLUDE_DIR}")
message(STATUS "lib_smoke: Taskflow include= ${TASKFLOW_INCLUDE_DIR}")
message(STATUS "lib_smoke: C++ standard    = ${CMAKE_CXX_STANDARD}")
message(STATUS "lib_smoke: WITH_FOLLY      = ${LIB_SMOKE_WITH_FOLLY}")
message(STATUS "lib_smoke: WITH_HPX        = ${LIB_SMOKE_WITH_HPX}")
message(STATUS "ll_headers: include/ll/*   = ON (SPSC, arena, TSC, affinity, pmr, hdr, sbe)")
message(STATUS "stack: industry hwloc/fb/mi/mc = ${STACK_HAS_HWLOC}/${STACK_HAS_FLATBUFFERS}/${STACK_HAS_MIMALLOC}/${STACK_HAS_MOODYCAMEL}")
if(LIB_SMOKE_WITH_HPX)
  message(STATUS "lib_smoke: HPX root        = ${LIB_SMOKE_HPX_ROOT}")
endif()
