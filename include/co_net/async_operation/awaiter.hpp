/**
 * @file awaiter.hpp
 * @author Roseveknif (rossqaq@outlook.com)
 * @brief 协程的具有特殊操作的 Awaiters
 * @version 0.1
 * @date 2024-03-31
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <concepts>
#include <coroutine>
#include <format>
#include <iostream>

namespace net::async::awaiters {

template <typename AwaiterType>
concept Awaiter = requires(AwaiterType awaiter, std::coroutine_handle<> h) {
    { awaiter.await_ready() } -> std::convertible_to<bool>;
    { awaiter.await_suspend(h) };
    { awaiter.await_resume() };
};

template <typename AwaitableType>
concept Awaitable = requires(AwaitableType a) {
    { a.operator co_await() } -> Awaiter;
};

struct FinalTaskAwaiter {
    constexpr bool await_ready() const noexcept { return false; }

    template <typename Promise>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> this_coro) const noexcept {
        if (this_coro.promise().caller_) {
            return this_coro.promise().caller_;
        } else {
            if (this_coro.promise().exp_) [[unlikely]] {
                std::rethrow_exception(this_coro.promise().exp_);
            }
        }
        return std::noop_coroutine();
    }

    constexpr void await_resume() const noexcept {}
};

struct SleepAwaiter {};

}  // namespace net::async::awaiters