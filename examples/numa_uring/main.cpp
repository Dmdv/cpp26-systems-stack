// Example: print NUMA / io_uring backend status (full path on Linux with libs).
#include "ll/linux_numa.hpp"
#include "ll/linux_uring.hpp"

#include <cstdio>

int main() {
  std::printf("%s\n", ll::numa::status_line().c_str());
  std::printf("%s\n", ll::uring::status_line().c_str());

  ll::uring::Ring ring;
  if (ring.open(16) && ring.submit_nop_and_wait()) {
    std::printf("uring: NOP completed ok\n");
  } else {
    std::printf("uring: skipped or unavailable on this platform\n");
  }

  if (ll::numa::available()) {
    std::printf("numa: preferred_node=%d max_node=%d\n", ll::numa::preferred_node(),
                ll::numa::max_node());
  } else {
    std::printf("numa: not available (install libnuma-dev on Linux)\n");
  }
  return 0;
}
