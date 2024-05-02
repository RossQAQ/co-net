#pragma once

#include <liburing.h>

#include <tuple>

#include "co_net/async/task.hpp"
#include "co_net/io/prep/completion_token.hpp"
#include "co_net/io/uring.hpp"

namespace net::io {

template <typename T>
struct UringAwaiter {
public:
    template <typename F>
        requires std::is_invocable_v<F, io_uring_sqe*>
    explicit UringAwaiter(io::Uring* ring, F&& func) {
        sqe_ = ring->get_sqe();
        func(sqe_);
        io_uring_sqe_set_data(sqe_, &token_);
    }

    UringAwaiter(const UringAwaiter&) = delete;

    UringAwaiter& operator=(const UringAwaiter&) = delete;

    UringAwaiter(UringAwaiter&&) = delete;

    UringAwaiter& operator=(UringAwaiter&&) = delete;

public:
    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> caller) {
        token_.handle_ = caller;
        return;
    }

    AsyncResult await_resume() noexcept { return { token_.op_, token_.cqe_res_, token_.cqe_flags_ }; }

protected:
    io_uring_sqe* sqe_{ nullptr };

    CompletionToken token_{};
};

}  // namespace net::io
