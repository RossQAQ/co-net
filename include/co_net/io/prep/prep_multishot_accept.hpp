#pragma once

#include <liburing.h>
#include <sys/socket.h>

#include "co_net/context/context.hpp"
#include "co_net/io/prep/uring_awaiter.hpp"

namespace net::io::operation {

class MultishotAcceptAwaiter : public net::io::UringAwaiter<MultishotAcceptAwaiter> {
public:
    using UringAwaiter = net::io::UringAwaiter<MultishotAcceptAwaiter>;

    template <typename F>
        requires std::is_invocable_v<F, io_uring_sqe*>
    MultishotAcceptAwaiter(io::Uring* ring, F&& func) : UringAwaiter(ring, std::forward<F>(func)) {
        token_.op_ = Op::MultishotAccept;
    }

public:
    void await_resume() {}
};

inline net::async::Task<void> prep_multishot_accept(int socket, int flags) {
    co_await MultishotAcceptAwaiter{ ::this_ctx::local_uring_loop, [&](io_uring_sqe* sqe) {
                                        io_uring_prep_multishot_accept(sqe, socket, nullptr, nullptr, flags);
                                    } };
}

}  // namespace net::io::operation