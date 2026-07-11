#pragma once

// Umbrella header: portable low-latency primitives + feature macros for
// optional industry libraries (hwloc, FlatBuffers, mimalloc, moodycamel, …).
//
// Feature macros (set by CMake):
//   LL_HAS_HWLOC, LL_HAS_FLATBUFFERS, LL_HAS_MIMALLOC, LL_HAS_JEMALLOC,
//   LL_HAS_BENCHMARK, LL_HAS_MOODYCAMEL, LL_HAS_STRUCT_PACK,
//   LL_HAS_HDRHISTOGRAM_C, LL_HAS_LIBNUMA, LL_HAS_LIBURING,
//   LL_HAS_SBE_CODEGEN, LL_HAS_DPDK, LL_HAS_ONLOAD, LL_HAS_FOLLY_PCQ

#include "ll/affinity.hpp"
#include "ll/arena.hpp"
#include "ll/branch.hpp"
#include "ll/cache_line.hpp"
#include "ll/hdr_c.hpp"
#include "ll/hdr_histogram.hpp"
#include "ll/jemalloc_util.hpp"
#include "ll/kernel_bypass.hpp"
#include "ll/linux_numa.hpp"
#include "ll/linux_uring.hpp"
#include "ll/pmr_arena.hpp"
#include "ll/sbe_style.hpp"
#include "ll/spsc_queue.hpp"
#include "ll/struct_pack_tick.hpp"
#include "ll/tsc_clock.hpp"

#ifndef LL_HAS_HWLOC
#define LL_HAS_HWLOC 0
#endif
#ifndef LL_HAS_FLATBUFFERS
#define LL_HAS_FLATBUFFERS 0
#endif
#ifndef LL_HAS_MIMALLOC
#define LL_HAS_MIMALLOC 0
#endif
#ifndef LL_HAS_JEMALLOC
#define LL_HAS_JEMALLOC 0
#endif
#ifndef LL_HAS_BENCHMARK
#define LL_HAS_BENCHMARK 0
#endif
#ifndef LL_HAS_MOODYCAMEL
#define LL_HAS_MOODYCAMEL 0
#endif
#ifndef LL_HAS_STRUCT_PACK
#define LL_HAS_STRUCT_PACK 0
#endif
#ifndef LL_HAS_HDRHISTOGRAM_C
#define LL_HAS_HDRHISTOGRAM_C 0
#endif
#ifndef LL_HAS_LIBNUMA
#define LL_HAS_LIBNUMA 0
#endif
#ifndef LL_HAS_LIBURING
#define LL_HAS_LIBURING 0
#endif
#ifndef LL_HAS_SBE_CODEGEN
#define LL_HAS_SBE_CODEGEN 0
#endif
#ifndef LL_HAS_DPDK
#define LL_HAS_DPDK 0
#endif
#ifndef LL_HAS_ONLOAD
#define LL_HAS_ONLOAD 0
#endif
#ifndef LL_HAS_FOLLY_PCQ
#define LL_HAS_FOLLY_PCQ 0
#endif

#if LL_HAS_SBE_CODEGEN
#include "ll/sbe_codec.hpp"
#endif
