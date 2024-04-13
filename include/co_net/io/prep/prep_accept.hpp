#pragma once

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
    AcceptAwaiter(io::Uring& ring, F&& func) : UringAwaiter(ring, std::forward<F>(func)) {}
};

inline net::async::Task<int> prep_accept(int socket, sockaddr* addr, socklen_t* addrlen, int flags) {
    auto [res, flag] = co_await AcceptAwaiter{ net::context::loop.get_uring_loop(), [&](io_uring_sqe* sqe) {
                                                  io_uring_prep_accept(sqe, socket, addr, addrlen, flags);
                                              } };

    if (res < 0) [[unlikely]] {
        throw std::runtime_error("io_uring prep accept failed.");
    }

    co_return res;
}

}  // namespace net::io::operation