#pragma once

#include <liburing.h>

#include "co_net/context/context.hpp"
#include "co_net/io/prep/uring_awaiter.hpp"

namespace net::io::operation {

class CloseAwaiter : public net::io::UringAwaiter<CloseAwaiter> {
public:
    using UringAwaiter = net::io::UringAwaiter<CloseAwaiter>;

    template <typename F>
        requires std::is_invocable_v<F, io_uring_sqe*>
    CloseAwaiter(io::Uring* ring, F&& func) : UringAwaiter(ring, std::forward<F>(func)) {
        token_.op_ = Op::Close;
    }
};

inline net::async::Task<void> prep_close_socket(int socket) {
    auto [op, res, flag] = co_await CloseAwaiter{ ::this_ctx::local_uring_loop,
                                                  [&](io_uring_sqe* sqe) { io_uring_prep_close(sqe, socket); } };

    if (res < 0) [[unlikely]] {
        throw std::runtime_error("io_uring close socket failed.");
    }
}

}  // namespace net::io::operation