#pragma once

#include <liburing.h>
#include <sys/socket.h>

#include <functional>

#include "co_net/context/context.hpp"
#include "co_net/context/this_ctx.hpp"
#include "co_net/io/prep/uring_awaiter.hpp"
#include "co_net/io/uring_msg.hpp"

namespace net::io::operation {

class RingMsgAwaiter : public net::io::UringAwaiter<RingMsgAwaiter> {
public:
    using UringAwaiter = net::io::UringAwaiter<RingMsgAwaiter>;

    template <typename F>
        requires std::is_invocable_v<F, io_uring_sqe*>
    RingMsgAwaiter(io::Uring* ring, F&& func) : UringAwaiter(ring, std::forward<F>(func)) {}
};

inline net::async::Task<int> prep_ring_msg(int receiver_fd, async::Task<void>&& task) {
    auto msg = new ::net::io::msg::RingMsg{ task.take() };

    auto [res, flag] = co_await RingMsgAwaiter{
        this_ctx::local_uring_loop,
        [&](io_uring_sqe* sqe) { io_uring_prep_msg_ring(sqe, receiver_fd, 0x5e2d, reinterpret_cast<uint64_t>(msg), 0); }
    };

    if (res < 0) [[unlikely]] {
        throw std::runtime_error("io_uring prep ring msg failed.");
    }

    co_return res;
}

}  // namespace net::io::operation