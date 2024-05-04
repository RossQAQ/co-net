#pragma once

#include <algorithm>
#include <list>

#include "co_net/async/scheduled_task.hpp"
#include "co_net/async/task.hpp"
#include "co_net/util/noncopyable.hpp"

namespace net::context {

class TaskLoop : public tools::Noncopyable {
public:
    ~TaskLoop() = default;

    void run() {
        for (auto& task : task_queue_) {
            if (task.valid() && !task.pending()) [[likely]] {
                task.resume();
            }
        }

        // task_queue_.erase(
        //     std::remove_if(task_queue_.begin(),
        //                    task_queue_.end(),
        //                    [](net::async::ScheduledTask& task) { return !task.valid() && !task.pending(); }),
        //     task_queue_.end());
    }

    void enqueue(net::async::Task<>&& task) { task_queue_.emplace_back(std::move(task)); }

    template <typename VoidTask, typename... Args>
    void emplace_back(VoidTask&& task, Args&&... args) {
        task_queue_.emplace_back(std::move(task), std::forward<Args>(args)...);
    }

    [[nodiscard]]
    bool empty() const noexcept {
        return task_queue_.empty();
    }

    [[nodiscard]]
    size_t size() const noexcept {
        return task_queue_.size();
    }

private:
    std::list<net::async::ScheduledTask> task_queue_;
};
}  // namespace net::context