#pragma once

#include <liburing.h>

#include "co_net/context/context.hpp"
#include "co_net/io/prep/uring_awaiter.hpp"

namespace net::io::operation {

class SocketAwaiter : public net::io::UringAwaiter<SocketAwaiter> {
public:
    using UringAwaiter = net::io::UringAwaiter<SocketAwaiter>;

    template <typename F>
        requires std::is_invocable_v<F, io_uring_sqe*>
    SocketAwaiter(io::Uring* ring, F&& func) : UringAwaiter(ring, std::forward<F>(func)) {
        token_.op_ = Op::Socket;
    }
};

inline net::async::Task<int> prep_normal_socket(int domain, int type, int protocol, int flags) {
    auto [res, flag] = co_await SocketAwaiter{ ::this_ctx::local_uring_loop, [&](io_uring_sqe* sqe) {
                                                  io_uring_prep_socket(sqe, domain, type, protocol, flags);
                                              } };

    if (res == -ENFILE) [[unlikely]] {
        throw std::runtime_error("io_uring prep normal socket failed.");
    }

    co_return res;
}

}  // namespace net::io::operation