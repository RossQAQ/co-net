#pragma once

#include <deque>

#include "co_net/async/task.hpp"
#include "co_net/util/noncopyable.hpp"

namespace net::context {
class TaskLoop : public tools::Noncopyable {
public:
    void run() {
        while (!task_queue_.empty()) {
            auto task = task_queue_.front();
            task_queue_.pop_front();
            task.resume();
        }
    }

    void enqueue(std::coroutine_handle<> coro) { task_queue_.push_back(coro); }

    [[nodiscard]]
    bool empty() const noexcept {
        return task_queue_.empty();
    }

    [[nodiscard]]
    size_t size() const noexcept {
        return task_queue_.size();
    }

private:
    std::deque<std::coroutine_handle<>> task_queue_;
};
}  // namespace net::context