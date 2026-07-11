// Example: poll-mode RX abstraction with stub backend (no NIC SDK required).
#include "ll/kernel_bypass.hpp"
#include "ll/sbe_codec.hpp"

#include <array>
#include <cstdio>
#include <cstring>
#include <span>

int main() {
  std::printf("bypass status: %s\n", ll::bypass::backend_status());

  ll::bypass::StubPollModeRx rx;
  rx.open(/*port*/ 0, /*queue*/ 0);

  // Simulate NIC delivering an SBE tick payload (body only for demo inject).
  ll::sbe_gen::TickValues tick{.instrument_id = 7,
                               .price_ticks = 100,
                               .qty = 1,
                               .seq = 1,
                               .ts_ns = 1};
  std::array<char, 64> wire{};
  const auto n = ll::sbe_gen::encode_tick(wire, tick);
  rx.inject_for_test(std::as_bytes(std::span(wire.data(), n)));

  ll::bypass::PacketView batch[8]{};
  const auto got = rx.poll(batch);
  std::printf("poll got=%zu bytes=%zu\n", got, got ? batch[0].length : 0);

  if (got) {
    ll::sbe_gen::TickValues out{};
    const char* p = reinterpret_cast<const char*>(batch[0].data);
    if (ll::sbe_gen::decode_tick(std::span<const char>(p, batch[0].length), out)) {
      std::printf("decoded sbe instrument=%u seq=%u\n", out.instrument_id, out.seq);
    }
  }

  std::printf(
      "Next: implement IPollModeRx with DPDK rte_eth_rx_burst or Onload/EF_VI on lab NIC.\n");
  return 0;
}
