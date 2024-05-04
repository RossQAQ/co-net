#pragma once

#include <liburing.h>

#include "co_net/context/context.hpp"
#include "co_net/io/prep/uring_awaiter.hpp"

namespace net::io::operation {

class DirectCloseAwaiter : public net::io::UringAwaiter<DirectCloseAwaiter> {
public:
    using UringAwaiter = net::io::UringAwaiter<DirectCloseAwaiter>;

    template <typename F>
        requires std::is_invocable_v<F, io_uring_sqe*>
    DirectCloseAwaiter(io::Uring* ring, F&& func) : UringAwaiter(ring, std::forward<F>(func)) {
        token_.op_ = Op::AsyncCloseDirect;
    }
};

inline net::async::Task<void> prep_close_socket_direct(int socket_direct,
                                                       net::io::Uring* = ::this_ctx::local_uring_loop) {
    auto [res, flag] = co_await DirectCloseAwaiter{ ::this_ctx::local_uring_loop, [&](io_uring_sqe* sqe) {
                                                       io_uring_prep_close_direct(sqe, socket_direct);
                                                   } };

    if (res < 0) [[unlikely]] {
        throw std::runtime_error("io_uring close direct socket failed.");
    }
}

}  // namespace net::io::operation