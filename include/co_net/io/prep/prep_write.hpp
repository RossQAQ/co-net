#pragma once

#include <liburing.h>

#include <span>

#include "co_net/context/context.hpp"
#include "co_net/io/prep/uring_awaiter.hpp"

namespace net::io::operation {

class WriteAwaiter : public net::io::UringAwaiter<WriteAwaiter> {
public:
    using UringAwaiter = net::io::UringAwaiter<WriteAwaiter>;

    template <typename F>
        requires std::is_invocable_v<F, io_uring_sqe*>
    WriteAwaiter(io::Uring* ring, F&& func) : UringAwaiter(ring, std::forward<F>(func)) {
        token_.op_ = Op::Send;
        sqe_->flags |= IOSQE_FIXED_FILE;
    }
};

inline net::async::Task<int> prep_write(int socket, std::span<char> buffer) {
    auto [res, flag] = co_await WriteAwaiter{ ::this_ctx::local_uring_loop, [&](io_uring_sqe* sqe) {
                                                 io_uring_prep_write(sqe, socket, buffer.data(), buffer.size(), 0);
                                             } };

    if (res < 0) [[unlikely]] {
        throw std::runtime_error("io_uring prep write failed.");
    }

    co_return res;
}

inline net::async::Task<int> prep_write(int socket, std::span<char> buffer, size_t len) {
    auto [res, flag] = co_await WriteAwaiter{ ::this_ctx::local_uring_loop, [&](io_uring_sqe* sqe) {
                                                 io_uring_prep_write(sqe, socket, buffer.data(), len, 0);
                                             } };

    if (res < 0) [[unlikely]] {
        throw std::runtime_error("io_uring prep write failed.");
    }

    co_return res;
}

}  // namespace net::io::operation