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
    DirectSocketAwaiter(io::Uring& ring, F&& func) : UringAwaiter(ring, std::forward<F>(func)) {}
};

inline net::async::Task<int> alloc_direct_socket(int domain, int type, int protocol, int flags) {
    auto [res, flag] =
        co_await DirectSocketAwaiter{ net::context::loop.get_uring_loop(), [&](io_uring_sqe* sqe) {
                                         io_uring_prep_socket_direct_alloc(sqe, domain, type, protocol, flags);
                                     } };

    // Need to prep fd tables first, then call this.
    if (res == -ENFILE) [[unlikely]] {
        throw std::runtime_error("io_uring direct socket allocation failed.");
    }

    co_return res;
}

}  // namespace net::io::operation