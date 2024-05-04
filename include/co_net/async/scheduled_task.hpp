#pragma once

#include <coroutine>
#include <exception>
#include <functional>
#include <stdexcept>
#include <utility>

#include "co_net/async/task.hpp"

namespace net::async {

class ScheduledTask {
public:
    ScheduledTask(net::async::Task<>&& task) : task_(std::move(task)), awaiting_handle_(task_.handle()) {
        task_.set_chain_task_root(this);
        // Dump(), task_.handle().address();
    }

    template <typename VoidTask, typename... Args>
        requires(std::is_invocable_r_v<net::async::Task<>, VoidTask, Args...> &&
                 std::is_rvalue_reference_v<VoidTask &&>)
    ScheduledTask(VoidTask&& task, Args&&... args) :
        task_(std::invoke(task, std::forward<Args>(args)...)),
        awaiting_handle_(task_.handle()) {
        task_.set_chain_task_root(this);
        // Dump(), "Coroutine: ", task_.handle().address();
        // Dump(), "Coroutine Promise: ", static_cast<void*>(std::addressof(task_.handle().promise()));
        // Dump(), "Scheduled Task: ", static_cast<void*>(this);
    }

    ScheduledTask(ScheduledTask&&) noexcept = default;

    ScheduledTask& operator=(ScheduledTask&&) noexcept = default;

    ~ScheduledTask() = default;

    void set_awaiting_handle(std::coroutine_handle<> next) { awaiting_handle_ = next; }

    void resume() {
        awaiting_handle_.resume();
        if (task_.done()) [[unlikely]] {
            valid_ = false;
        }
    }

    [[nodiscard]]
    bool valid() noexcept {
        return valid_;
    }

    [[nodiscard]]
    bool pending() noexcept {
        return pending_;
    }

    auto handle() { return task_.handle(); }

    void set_pending(bool pending) { pending_ = pending; }

private:
    [[nodiscard]]
    bool done() {
        return task_.done();
    }

private:
    net::async::Task<> task_;

    std::coroutine_handle<> awaiting_handle_{ nullptr };

    bool valid_{ true };

    bool pending_{ false };
};

};  // namespace net::async