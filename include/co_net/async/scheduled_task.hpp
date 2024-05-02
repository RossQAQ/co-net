#pragma once

#include <coroutine>
#include <exception>
#include <stdexcept>
#include <utility>

#include "co_net/async/task.hpp"

namespace net::async {

class ScheduledTask {
public:
    ScheduledTask(net::async::Task<>&& task) : task_(std::move(task)), continuation_(task_.handle()) {
        task_.set_chian_task_root(this);
    }

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

    std::coroutine_handle<> continuation_;

    bool valid_{ true };

    bool pending_{ false };
};

};  // namespace net::async