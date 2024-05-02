#pragma once

#include <coroutine>
#include <exception>
#include <stdexcept>
#include <utility>

#include "co_net/async/awaiter.hpp"
#include "co_net/dump.hpp"

using tools::debug::Dump;

namespace net::async {

class ScheduledTask;

struct BasicPromise {
    auto initial_suspend() const noexcept { return std::suspend_always{}; }

    void unhandled_exception() noexcept { exp_ = std::current_exception(); }

    void set_caller(std::coroutine_handle<> caller) { caller_ = caller; }

    void set_chain_root(ScheduledTask* task) { this_chain_task_ = task; }

    std::coroutine_handle<> caller_{ nullptr };

    std::exception_ptr exp_{ nullptr };

    ScheduledTask* this_chain_task_{ nullptr };
};

}  // namespace net::async
