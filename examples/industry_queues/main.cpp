// Example: industry SPSC options side-by-side with ll::SpscQueue.
#include "ll/spsc_queue.hpp"

#include <boost/lockfree/spsc_queue.hpp>

#include <cstdio>
#include <thread>

#if LL_HAS_MOODYCAMEL
#include "readerwriterqueue.h"
#endif

int main() {
  {
    ll::SpscQueue<int, 64> q;
    (void)q.try_push(1);
    auto v = q.try_pop();
    std::printf("ll::SpscQueue pop=%d\n", v ? *v : -1);
  }
  {
    boost::lockfree::spsc_queue<int, boost::lockfree::capacity<64>> q;
    q.push(2);
    int v = 0;
    q.pop(v);
    std::printf("boost::lockfree::spsc_queue pop=%d\n", v);
  }
#if LL_HAS_MOODYCAMEL
  {
    moodycamel::ReaderWriterQueue<int> q(64);
    q.try_enqueue(3);
    int v = 0;
    q.try_dequeue(v);
    std::printf("moodycamel::ReaderWriterQueue pop=%d\n", v);
  }
#else
  std::printf("moodycamel: not built (LL_HAS_MOODYCAMEL=0)\n");
#endif
  return 0;
}
