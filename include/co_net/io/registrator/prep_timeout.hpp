#pragma once

#include <liburing.h>

#include "co_net/context/context.hpp"
#include "co_net/io/registrator/uring_awaiter.hpp"

namespace net::io::operation {

class TimeoutAwaiter : public net::io::UringAwaiter<TimeoutAwaiter> {
public:
    using UringAwaiter = net::io::UringAwaiter<TimeoutAwaiter>;

    template <typename F>
        requires std::is_invocable_v<F, io_uring_sqe*>
    TimeoutAwaiter(io::Uring& ring, F&& func) : UringAwaiter(ring, std::forward<F>(func)) {}

    void await_resume() noexcept {}

private:
    io_uring_sqe* sqe_{ nullptr };
};

inline net::async::Task<void> uring_prep_timer_once(__kernel_timespec ts,
                                                    int count = 1,
                                                    int flags = IORING_TIMEOUT_REALTIME |
                                                                IORING_TIMEOUT_ETIME_SUCCESS) {
    co_await TimeoutAwaiter{ net::context::loop.get_uring_loop(),
                             [&](io_uring_sqe* sqe) { io_uring_prep_timeout(sqe, &ts, count, flags); } };
}

}  // namespace net::io::operation