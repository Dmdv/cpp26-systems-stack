#pragma once

#include <string>

#if defined(__linux__)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>
#include <sched.h>
#elif defined(__APPLE__)
#include <pthread.h>
#endif

namespace ll {

// Thread placement helpers for low-latency workers.
// Linux: real CPU affinity via pthread_setaffinity_np.
// macOS: QoS class only (no public core-pin API equivalent).
//
// See docs/blueprint/01-hardware-os.md
inline bool pin_current_thread_to_cpu(int cpu_index) {
#if defined(__linux__)
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(cpu_index, &set);
  return pthread_setaffinity_np(pthread_self(), sizeof(set), &set) == 0;
#else
  (void)cpu_index;
  return false;  // not supported
#endif
}

inline bool set_current_thread_interactive_qos() {
#if defined(__APPLE__)
  return pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0) == 0;
#else
  return false;
#endif
}

inline const char* affinity_backend() {
#if defined(__linux__)
  return "linux-pthread_setaffinity_np";
#elif defined(__APPLE__)
  return "macos-qos-user-interactive";
#else
  return "none";
#endif
}

// Documented boot / process tips (no runtime effect) — for operators.
inline constexpr const char* kLinuxIsolationNotes = R"ops(
# Example Linux boot args (lab machines only — not default for laptops):
#   isolcpus=2,3 nohz_full=2,3 rcu_nocbs=2,3
# Run process under:
#   numactl --cpunodebind=0 --membind=0 ./app
# CPU governor:
#   cpupower frequency-set -g performance
# Hugepages (example 2MB):
#   echo 1024 > /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
)ops";

}  // namespace ll
