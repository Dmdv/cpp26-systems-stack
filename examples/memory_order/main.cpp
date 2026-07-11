// Example: document acquire/release pairing for a one-slot handoff flag.
#include <atomic>
#include <cstdio>
#include <thread>

// Incorrect seq_cst everywhere works but is slower on weak memory models.
// This shows the minimal correct pattern for "payload then flag".

struct Message {
  int value;
};

int main() {
  Message slot{};
  std::atomic<bool> ready{false};
  int observed = 0;

  std::thread producer([&] {
    slot.value = 42;                                  // 1. write payload
    ready.store(true, std::memory_order_release);     // 2. publish
  });

  std::thread consumer([&] {
    while (!ready.load(std::memory_order_acquire)) {  // 3. observe publish
    }
    observed = slot.value;                            // 4. read payload safely
  });

  producer.join();
  consumer.join();
  std::printf("memory_order example observed=%d\n", observed);
  return observed == 42 ? 0 : 1;
}
