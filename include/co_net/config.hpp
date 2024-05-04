#pragma once

#include <thread>

#include "co_net/timer/timer.hpp"

namespace config {

using namespace net::time_literals;

using thread_t = std::invoke_result_t<decltype(std::thread::hardware_concurrency)>;

inline constexpr thread_t WORKERS = 0;

inline constexpr size_t URING_MAIN_CQES = 1024;

inline constexpr size_t URING_MAIN_SQES = 128;

inline constexpr size_t URING_WOKERS_SQES = 512;

inline constexpr size_t URING_PEEK_CQES_BATCH = 256;

inline constexpr bool URING_USE_MULTISHOT_ACCEPT = false;

inline constexpr __kernel_timespec MINOR_URING_WAIT_CQE_TIMEOUT = 3_s;

inline constexpr __kernel_timespec WORKER_URING_WAIT_CQE_INTERVAL = 15_ms;

inline constexpr bool URING_USE_DIRECT_FD_AS_SOCKET = true;

inline constexpr size_t URING_DIRECT_FD_TABLE_SIZE = 32768;

inline constexpr size_t BUFFER_RING_SIZE = 512;

inline constexpr size_t N_BUFFERS = 1024 * 16;

inline constexpr bool AUTO_EXTEND_WHEN_NOBUFS = true;

inline constexpr __kernel_timespec NOBUFS_SLEEP_FOR = 15_ms;

}  // namespace config
