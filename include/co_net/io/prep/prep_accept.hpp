#pragma once

#include <arpa/inet.h>
#include <liburing.h>
#include <sys/socket.h>

#include "co_net/context/context.hpp"
#include "co_net/io/prep/uring_awaiter.hpp"

namespace net::io::operation {

class AcceptAwaiter : public net::io::UringAwaiter<AcceptAwaiter> {
public:
    using UringAwaiter = net::io::UringAwaiter<AcceptAwaiter>;

    template <typename F>
        requires std::is_invocable_v<F, io_uring_sqe*>
    AcceptAwaiter(io::Uring* ring, F&& func) : UringAwaiter(ring, std::forward<F>(func)) {
        token_.op_ = Op::Accept;
    }
};

inline net::async::Task<int> prep_accept(int socket, int flags) {
    auto [op, res, flag] = co_await AcceptAwaiter{ ::this_ctx::local_uring_loop, [&](io_uring_sqe* sqe) {
                                                      io_uring_prep_accept(sqe, socket, nullptr, nullptr, flags);
                                                  } };

    if (res < 0) [[unlikely]] {
        throw std::runtime_error("io_uring prep accept failed.");
    }

    co_return res;
}

class DirectAcceptAwaiter : public net::io::UringAwaiter<DirectAcceptAwaiter> {
public:
    using UringAwaiter = net::io::UringAwaiter<DirectAcceptAwaiter>;

    template <typename F>
        requires std::is_invocable_v<F, io_uring_sqe*>
    DirectAcceptAwaiter(io::Uring* ring, F&& func) : UringAwaiter(ring, std::forward<F>(func)) {
        token_.op_ = Op::Accept;
        sqe_->file_index |= IORING_FILE_INDEX_ALLOC;
    }
};

inline net::async::Task<int> prep_accept_direct(int socket_direct) {
    auto [op, res, flag] = co_await DirectAcceptAwaiter{
        ::this_ctx::local_uring_loop,
        [&](io_uring_sqe* sqe) {
            io_uring_prep_accept_direct(sqe, socket_direct, nullptr, nullptr, 0, IORING_FILE_INDEX_ALLOC);
        }
    };

    if (res < 0) [[unlikely]] {
        Dump(), strerror(-res), res;
        throw std::runtime_error("io_uring prep direct accept failed.");
    }

    co_return res;
}

}  // namespace net::io::operation