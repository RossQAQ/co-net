#pragma once

#include <thread>

namespace config {

using thread_t = std::invoke_result_t<decltype(std::thread::hardware_concurrency)>;

inline constexpr thread_t WORKERS = 0;

inline constexpr size_t URING_MAIN_CQES = 1024;

inline constexpr size_t URING_MAIN_SQES = 128;

inline constexpr size_t URING_WOKERS_SQES = 512;

inline constexpr size_t URING_PEEK_CQES_BATCH = 256;

inline constexpr bool URING_USE_MULTISHOT_ACCEPT = false;

// inline constexpr bool URING_USE_DIRECT_FD_AS_SOCKET = true;

// inline constexpr size_t URING_DIRECT_FD_TABLE_SIZE = 32768;

// 8mb
inline constexpr size_t PROVIDED_BUFFER_INITIAL_SIZE = 8'388'608;

}  // namespace config
