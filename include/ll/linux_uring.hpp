#pragma once

// Minimal io_uring (liburing) helpers for low-syscall async I/O on Linux.
// Not a full reactor — demonstrates setup, SQE prep, submit, CQE harvest.
// See docs/tutorials/linux-numa-uring.md

#include <cstdint>
#include <span>
#include <string>

#ifndef LL_HAS_LIBURING
#define LL_HAS_LIBURING 0
#endif

#if LL_HAS_LIBURING
#include <liburing.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#endif

namespace ll::uring {

[[nodiscard]] inline bool available() noexcept {
#if LL_HAS_LIBURING
  return true;
#else
  return false;
#endif
}

// RAII ring wrapper (Linux). Empty stub elsewhere.
class Ring {
 public:
  Ring() = default;
  Ring(const Ring&) = delete;
  Ring& operator=(const Ring&) = delete;

  ~Ring() { close(); }

  // queue_depth: SQ size (power of two recommended by kernel docs).
  [[nodiscard]] bool open(unsigned queue_depth = 32) noexcept {
#if LL_HAS_LIBURING
    close();
    if (io_uring_queue_init(queue_depth, &ring_, 0) != 0) {
      return false;
    }
    open_ = true;
    return true;
#else
    (void)queue_depth;
    return false;
#endif
  }

  void close() noexcept {
#if LL_HAS_LIBURING
    if (open_) {
      io_uring_queue_exit(&ring_);
      open_ = false;
    }
#endif
  }

  [[nodiscard]] bool is_open() const noexcept {
#if LL_HAS_LIBURING
    return open_;
#else
    return false;
#endif
  }

  // Submit a NOP (validates ring path without file dependencies).
  [[nodiscard]] bool submit_nop_and_wait() noexcept {
#if LL_HAS_LIBURING
    if (!open_) {
      return false;
    }
    io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
      return false;
    }
    io_uring_prep_nop(sqe);
    if (io_uring_submit(&ring_) < 0) {
      return false;
    }
    io_uring_cqe* cqe = nullptr;
    if (io_uring_wait_cqe(&ring_, &cqe) < 0) {
      return false;
    }
    const int res = cqe->res;
    io_uring_cqe_seen(&ring_, cqe);
    return res >= 0;
#else
    return false;
#endif
  }

  // Read file into buffer via io_uring (path must exist). Returns bytes read or -1.
  [[nodiscard]] int read_file(const char* path, std::span<char> dst) noexcept {
#if LL_HAS_LIBURING
    if (!open_ || !path || dst.empty()) {
      return -1;
    }
    const int fd = ::open(path, O_RDONLY);
    if (fd < 0) {
      return -1;
    }
    io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
      ::close(fd);
      return -1;
    }
    io_uring_prep_read(sqe, fd, dst.data(), static_cast<unsigned>(dst.size()), 0);
    if (io_uring_submit(&ring_) < 0) {
      ::close(fd);
      return -1;
    }
    io_uring_cqe* cqe = nullptr;
    if (io_uring_wait_cqe(&ring_, &cqe) < 0) {
      ::close(fd);
      return -1;
    }
    const int res = cqe->res;
    io_uring_cqe_seen(&ring_, cqe);
    ::close(fd);
    return res;
#else
    (void)path;
    (void)dst;
    return -1;
#endif
  }

 private:
#if LL_HAS_LIBURING
  io_uring ring_{};
  bool open_{false};
#endif
};

[[nodiscard]] inline const char* backend() noexcept {
#if LL_HAS_LIBURING
  return "liburing";
#else
  return "stub";
#endif
}

[[nodiscard]] inline std::string status_line() {
  std::string s = "uring_backend=";
  s += backend();
  s += " available=";
  s += available() ? "yes" : "no";
  return s;
}

}  // namespace ll::uring
