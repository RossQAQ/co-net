#pragma once

#include <coroutine>
#include <exception>
#include <stdexcept>
#include <utility>

#include "co_net/async/awaiter.hpp"
#include "co_net/async/promise_type.hpp"
#include "co_net/async/result.hpp"

namespace net::async {

template <typename T, typename Promise>
class Task;

template <typename T>
struct TaskPromise final : public BasicPromise {
    auto final_suspend() const noexcept { return awaiters::FinalTaskAwaiter{}; }

    auto get_return_object() { return std::coroutine_handle<TaskPromise>::from_promise(*this); }

    void return_value(const T& lval) { res_.construct_result(lval); }

    void return_value(T&& rval) { res_.construct_result(std::move(rval)); }

    [[nodiscard]]
    T result() {
        if (exp_) [[unlikely]] {
            std::rethrow_exception(exp_);
        }
        return res_.move_out_result();
    }

private:
    Result<T> res_;
};

template <>
struct TaskPromise<void> : public BasicPromise {
    auto final_suspend() const noexcept { return awaiters::FinalTaskAwaiter{}; }

    auto get_return_object() { return std::coroutine_handle<TaskPromise>::from_promise(*this); }

    void return_void() {}

    void result() {
        if (exp_) [[unlikely]] {
            std::rethrow_exception(exp_);
        }
    }
};

template <typename T = void, typename Promise = TaskPromise<T>>
class [[nodiscard]] Task {
public:
    using promise_type = Promise;

    struct TaskAwaier {
        std::coroutine_handle<promise_type> callee_{ nullptr };

        auto await_ready() const noexcept { return false; }

        template <typename CallerPromise>
        std::coroutine_handle<promise_type> await_suspend(std::coroutine_handle<CallerPromise> caller) const noexcept {
            callee_.promise().set_caller(caller);
            callee_.promise().set_chain_root(caller.promise().this_chain_task_);
            return callee_;
        }

        [[nodiscard]]
        T await_resume() const {
            return callee_.promise().result();
        }
    };

public:
    Task(std::coroutine_handle<promise_type> hnd) noexcept : handle_(hnd) {}

    Task(Task&) = delete;

    Task& operator=(const Task&) = delete;

    Task(Task&& rhs) noexcept : handle_(rhs.handle_) { rhs.handle_ = nullptr; }

    Task& operator=(Task&& rhs) noexcept {
        handle_ = std::exchange(rhs.handle_, nullptr);
        return *this;
    }

    ~Task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    [[nodiscard]]
    std::coroutine_handle<promise_type> handle() const noexcept {
        return handle_;
    }

    [[nodiscard]]
    std::coroutine_handle<promise_type> take() noexcept {
        return std::exchange(handle_, nullptr);
    }

    [[nodiscard]]
    auto
    operator co_await() const noexcept {
        return TaskAwaier{ handle_ };
    }

    [[nodiscard]] operator std::coroutine_handle<promise_type>() const noexcept { return handle_; }

    [[nodiscard]]
    bool done() const noexcept {
        return handle_.done();
    }

    void resume() { handle_.resume(); }

    void set_chain_task_root(ScheduledTask* root) { handle_.promise().set_chain_root(root); }

private:
    std::coroutine_handle<promise_type> handle_{ nullptr };
};

}  // namespace net::async
