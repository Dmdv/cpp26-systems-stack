# Minimal Linux CI build: SBE + NUMA + uring + HdrHistogram_c + bypass stubs.
# Invoked from .github/workflows/ci.yml — does not require the full systems mesh.
cmake_minimum_required(VERSION 3.20)
project(linux_roadmap_smoke LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
include(CTest)
enable_testing()

set(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../..")

find_package(Threads REQUIRED)

# Catch2
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.5.4
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

# HdrHistogram_c
set(HDR_HISTOGRAM_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(HDR_HISTOGRAM_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(HDR_HISTOGRAM_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(HDR_LOG_REQUIRED OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  hdr_histogram_c
  GIT_REPOSITORY https://github.com/HdrHistogram/HdrHistogram_c.git
  GIT_TAG        0.11.10
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(hdr_histogram_c)

find_library(NUMA_LIBRARY numa)
find_path(NUMA_INCLUDE_DIR numa.h)
find_library(URING_LIBRARY uring)
find_path(URING_INCLUDE_DIR liburing.h)

add_executable(test_roadmap_stack "${ROOT}/tests/test_roadmap_stack.cpp")
target_include_directories(test_roadmap_stack PRIVATE
  "${ROOT}/include"
  "${ROOT}/generated/sbe/market_sbe"
)
target_compile_definitions(test_roadmap_stack PRIVATE LL_HAS_HDRHISTOGRAM_C=1 LL_HAS_SBE_CODEGEN=1)
target_link_libraries(test_roadmap_stack PRIVATE Catch2::Catch2WithMain Threads::Threads hdr_histogram_static)

if(NUMA_LIBRARY AND NUMA_INCLUDE_DIR)
  target_compile_definitions(test_roadmap_stack PRIVATE LL_HAS_LIBNUMA=1)
  target_include_directories(test_roadmap_stack PRIVATE "${NUMA_INCLUDE_DIR}")
  target_link_libraries(test_roadmap_stack PRIVATE "${NUMA_LIBRARY}")
  message(STATUS "linux_roadmap: libnuma=ON")
else()
  message(STATUS "linux_roadmap: libnuma=OFF")
endif()

if(URING_LIBRARY AND URING_INCLUDE_DIR)
  target_compile_definitions(test_roadmap_stack PRIVATE LL_HAS_LIBURING=1)
  target_include_directories(test_roadmap_stack PRIVATE "${URING_INCLUDE_DIR}")
  target_link_libraries(test_roadmap_stack PRIVATE "${URING_LIBRARY}")
  message(STATUS "linux_roadmap: liburing=ON")
else()
  message(STATUS "linux_roadmap: liburing=OFF")
endif()

include(Catch)
catch_discover_tests(test_roadmap_stack)
