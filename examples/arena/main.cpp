// Example: arena allocation for temporary per-window state (no malloc on path).
#include "ll/arena.hpp"

#include <cstdio>
#include <string_view>
#include <vector>

struct Level {
  double px;
  double qty;
};

int main() {
  ll::Arena arena(1 << 16);
  for (int window = 0; window < 3; ++window) {
    arena.reset();
    auto* levels = static_cast<Level*>(arena.allocate(sizeof(Level) * 10, alignof(Level)));
    for (int i = 0; i < 10; ++i) {
      levels[i] = Level{100.0 + i, 1.0 * (i + 1)};
    }
    double notional = 0;
    for (int i = 0; i < 10; ++i) {
      notional += levels[i].px * levels[i].qty;
    }
    std::printf("window=%d notional=%.2f arena_used=%zu\n", window, notional,
                arena.used());
  }
  return 0;
}
