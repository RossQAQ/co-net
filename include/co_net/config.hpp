/**
 * @file config.hpp
 * @author Roseveknif (rossqaq@outlook.com)
 * @brief io_context 的 Config 文件，在构建 io_context 时需要
 * @version 0.1
 * @date 2024-04-05
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include <thread>

namespace config {

using thread_t = std::invoke_result_t<decltype(std::thread::hardware_concurrency)>;

inline constexpr thread_t WORKERS = 0;

inline constexpr size_t URING_MAIN_CQES = 1024;

inline constexpr size_t URING_MAIN_SQES = 128;

inline constexpr size_t URING_WOKERS_SQES = 256;

}  // namespace config
