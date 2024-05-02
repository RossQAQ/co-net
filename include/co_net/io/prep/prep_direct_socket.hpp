#pragma once

#include <liburing.h>

#include "co_net/context/context.hpp"
#include "co_net/io/prep/uring_awaiter.hpp"

namespace net::io::operation {

class DirectSocketAwaiter : public net::io::UringAwaiter<DirectSocketAwaiter> {
public:
    using UringAwaiter = net::io::UringAwaiter<DirectSocketAwaiter>;

    template <typename F>
        requires std::is_invocable_v<F, io_uring_sqe*>
    DirectSocketAwaiter(io::Uring* ring, F&& func) : UringAwaiter(ring, std::forward<F>(func)) {
        token_.op_ = Op::Socket;
        if constexpr (config::URING_DIRECT_FD_TABLE_SIZE) {
            sqe_->flags |= IOSQE_FIXED_FILE;
        }
    }
};

inline net::async::Task<int> prep_direct_socket(int domain, int type, int protocol, int flags) {
    auto [op, res, flag] =
        co_await DirectSocketAwaiter{ ::this_ctx::local_uring_loop, [&](io_uring_sqe* sqe) {
                                         io_uring_prep_socket_direct_alloc(sqe, domain, type, protocol, flags);
                                     } };

    if (res < 0) [[unlikely]] {
        throw std::runtime_error("io_uring direct socket allocation failed.");
    }

    co_return res;
}

}  // namespace net::io::operation