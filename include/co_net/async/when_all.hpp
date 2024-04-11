/**
 * @file when_all.hpp
 * @author Roseveknif (rossqaq@outlook.com)
 * @brief 协程实现 when_all 功能
 * @version 0.1
 * @date 2024-04-05
 *
 * @copyright Copyright (c) 2024
 *
 */

#include "co_net/async/awaiter.hpp"
#include "co_net/async/task.hpp"

namespace net::async {

struct WhenAllControlBlock {
    size_t coro_cnt_{};
    std::coroutine_handle<> caller_;
    std::exception_ptr exp_;
};

struct WhenAllAwaiter {
    constexpr bool await_ready() const noexcept { return false; }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<> caller) noexcept {}

    void await_resume() const noexcept {}

    Task<void> t1;
    Task<void> t2;
};

inline Task<void> when_all(const Task<void>& t1, const Task<void>& t2) {
    WhenAllControlBlock cb;
    cb.coro_cnt_ = 2;
    co_await WhenAllAwaiter{ t1, t2 };
}

inline Task<void> when_all_executor(const Task<void>& task, WhenAllControlBlock& cb) {
    co_await task;
    cb.coro_cnt_--;
    if (cb.coro_cnt_ == 0) {}
}

}  // namespace net::async
