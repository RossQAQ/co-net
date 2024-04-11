/**
 * @file scheduler.hpp
 * @author Roseveknif (rossqaq@outlook.com)
 * @brief Scheduler 调度器实现。
 * @version 0.1
 * @date 2024-04-02
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <coroutine>
#include <queue>

#include "co_net/async/task.hpp"
#include "co_net/dump.hpp"
#include "co_net/util/noncopyable.hpp"

namespace net::context {

class Scheduler : public tools::Noncopyable {
public:
    Scheduler() = default;

    ~Scheduler() = default;

    void submit(std::coroutine_handle<> task) { awaited_.push(task); }

    void scheduler_one() {
        if (!awaited_.empty()) {
            auto handle = awaited_.front();
            awaited_.pop();
            handle.resume();
            if (!handle.done()) {
                this->submit(handle);
            }
        }
    }

    void loop() {
        while (!awaited_.empty()) {
            auto handle = awaited_.front();
            awaited_.pop();
            handle.resume();
            if (!handle.done()) {
                this->submit(handle);
            }
        }
    }

private:
    std::queue<std::coroutine_handle<>> awaited_;

    std::queue<std::coroutine_handle<>> timed_;
};

}  // namespace net::context
