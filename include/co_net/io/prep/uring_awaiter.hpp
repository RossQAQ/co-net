#pragma once

#include <liburing.h>

#include <tuple>

#include "co_net/async/scheduled_task.hpp"
#include "co_net/async/task.hpp"
#include "co_net/io/prep/completion_token.hpp"
#include "co_net/io/uring.hpp"

namespace net::io {

template <typename T>
struct UringAwaiter {
public:
    template <typename F>
        requires std::is_invocable_v<F, io_uring_sqe*>
    explicit UringAwaiter(net::io::Uring* ring, F&& func) {
        sqe_ = ring->get_sqe();
        func(sqe_);
        io_uring_sqe_set_data(sqe_, static_cast<void*>(&token_));
    }

    UringAwaiter(const UringAwaiter&) = delete;

    UringAwaiter& operator=(const UringAwaiter&) = delete;

    UringAwaiter(UringAwaiter&&) = delete;

    UringAwaiter& operator=(UringAwaiter&&) = delete;

public:
    bool await_ready() const noexcept { return false; }

    template <typename Promise>
    void await_suspend(std::coroutine_handle<Promise> caller) {
        caller.promise().this_chain_task_->set_awaiting_handle(caller);
        token_.chain_task_ = caller.promise().this_chain_task_;
        token_.chain_task_->set_pending(true);
        // Dump(), "Chain Task in Uring Awaiter: ", static_cast<void*>(token_.chain_task_);
        return;
    }

    std::tuple<int, int> await_resume() noexcept { return std::make_tuple(token_.cqe_res_, token_.cqe_flags_); }

protected:
    io_uring_sqe* sqe_{ nullptr };

    CompletionToken token_{};
};

}  // namespace net::io
