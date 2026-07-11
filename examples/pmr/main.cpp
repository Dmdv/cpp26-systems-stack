// Example: std::pmr monotonic arena for per-window scratch (no malloc on path).
#include "ll/pmr_arena.hpp"

#include <cstdio>
#include <string_view>

struct OrderView {
  std::string_view symbol;
  double px;
  double qty;
};

int main() {
  ll::PmrMonotonicArena arena(1 << 16);
  for (int w = 0; w < 3; ++w) {
    auto* o1 = arena.create<OrderView>(OrderView{"AAPL", 190.0 + w, 10});
    auto* o2 = arena.create<OrderView>(OrderView{"MSFT", 400.0 + w, 5});
    std::printf("window=%d %.*s@%.1f %.*s@%.1f\n", w,
                static_cast<int>(o1->symbol.size()), o1->symbol.data(), o1->px,
                static_cast<int>(o2->symbol.size()), o2->symbol.data(), o2->px);
    arena.release();  // reclaim entire window
  }
  return 0;
}
