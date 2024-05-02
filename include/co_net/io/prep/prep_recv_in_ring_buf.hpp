#pragma once

#include <liburing.h>

#include <span>

#include "co_net/context/context.hpp"
#include "co_net/io/prep/uring_awaiter.hpp"

namespace net::io::operation {

class RecvRingBufAwaiter : public net::io::UringAwaiter<RecvRingBufAwaiter> {
public:
    using UringAwaiter = net::io::UringAwaiter<RecvRingBufAwaiter>;

    template <typename F>
        requires std::is_invocable_v<F, io_uring_sqe*>
    RecvRingBufAwaiter(io::Uring* ring, F&& func) : UringAwaiter(ring, std::forward<F>(func)) {
        token_.op_ = Op::Receive;
        sqe_->buf_group = 0;
        sqe_->flags |= IOSQE_BUFFER_SELECT;
        sqe_->flags |= IOSQE_FIXED_FILE;
    }
};

inline net::async::Task<std::tuple<int, int>> prep_recv_in_ring_buf(int socket) {
    auto [op, res, flag] =
        co_await RecvRingBufAwaiter{ ::this_ctx::local_uring_loop, [&](io_uring_sqe* sqe) {
                                        io_uring_prep_recv(sqe, socket, nullptr, config::BUFFER_RING_SIZE, 0);
                                    } };

    if (res < 0 && res != -ENOBUFS) [[unlikely]] {
        throw std::runtime_error("io_uring prep receive with ring buffer failed.");
    }

    co_return { res, flag };
}

}  // namespace net::io::operation