/**
 * @file promise_type.hpp
 * @author Roseveknif (rossqaq@outlook.com)
 * @brief 协程的 promise_type 实现
 * @version 0.1
 * @date 2024-03-31
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <coroutine>
#include <exception>
#include <stdexcept>
#include <utility>

#include "co_net/async/awaiter.hpp"
#include "co_net/dump.hpp"

using tools::debug::Dump;

namespace net::async {

struct BasicPromise {
    auto initial_suspend() const noexcept { return std::suspend_always{}; }

    void unhandled_exception() noexcept { exp_ = std::current_exception(); }

    void set_caller(std::coroutine_handle<> caller) { caller_ = caller; }

    std::coroutine_handle<> caller_{ nullptr };

    std::exception_ptr exp_{ nullptr };
};

}  // namespace net::async
