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
    ScheduledTask(net::async::Task<>&& task) : task_(std::move(task)), continuation_(task_.handle()) {
        task_.set_chain_task_root(this);
    }

    template <typename VoidTask, typename... Args>
        requires(std::is_invocable_r_v<net::async::Task<>, VoidTask, Args...> &&
                 std::is_rvalue_reference_v<VoidTask &&>)
    ScheduledTask(VoidTask&& task, Args&&... args) :
        task_(std::invoke(task, std::forward<Args>(args)...)),
        continuation_(task_.handle()) {
        task_.set_chain_task_root(this);
    }

    ScheduledTask(ScheduledTask&&) = default;

    ScheduledTask& operator=(ScheduledTask&&) = default;

    ~ScheduledTask() = default;

    void set_continuation(std::coroutine_handle<> next) { continuation_ = next; }

    void resume() {
        if (task_.done()) {
            valid_ = false;
            return;
        } else [[likely]] {
            if (!pending_) {
                continuation_.resume();
            }
        }
    }

    [[nodiscard]]
    bool valid() noexcept {
        return valid_;
    }

    [[nodiscard]]
    bool done() {
        return task_.done();
    }

    void set_pending(bool pending) { pending_ = pending; }

private:
    net::async::Task<> task_;

    std::coroutine_handle<> continuation_{ nullptr };

    bool valid_{ true };

    bool pending_{ false };
};

};  // namespace net::async