// Google Benchmark: compare ll::SpscQueue vs boost::lockfree::spsc_queue
// (and moodycamel when available). Run: ./build/bench_queues
//
// Measures enqueue+dequeue pairs on a single thread (best-case path cost).

#include <benchmark/benchmark.h>

#include "ll/spsc_queue.hpp"

#include <boost/lockfree/spsc_queue.hpp>

#if LL_HAS_MOODYCAMEL
#include "readerwriterqueue.h"
#endif

static void BM_ll_spsc_push_pop(benchmark::State& state) {
  ll::SpscQueue<int, 1024> q;
  int sink = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(q.try_push(1));
    auto v = q.try_pop();
    if (v) {
      sink += *v;
    }
    benchmark::DoNotOptimize(sink);
  }
}
BENCHMARK(BM_ll_spsc_push_pop);

static void BM_boost_spsc_push_pop(benchmark::State& state) {
  boost::lockfree::spsc_queue<int, boost::lockfree::capacity<1024>> q;
  int sink = 0;
  int v = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(q.push(1));
    if (q.pop(v)) {
      sink += v;
    }
    benchmark::DoNotOptimize(sink);
  }
}
BENCHMARK(BM_boost_spsc_push_pop);

#if LL_HAS_MOODYCAMEL
static void BM_moodycamel_push_pop(benchmark::State& state) {
  moodycamel::ReaderWriterQueue<int> q(1024);
  int sink = 0;
  int v = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(q.try_enqueue(1));
    if (q.try_dequeue(v)) {
      sink += v;
    }
    benchmark::DoNotOptimize(sink);
  }
}
BENCHMARK(BM_moodycamel_push_pop);
#endif

BENCHMARK_MAIN();
