#pragma once

// ! broken

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
    MultishotAcceptAwaiter(io::Uring& ring, F&& func) : UringAwaiter(ring, std::forward<F>(func)) {
        state = ture;
    }
};

// ! todo
inline net::async::Task<int> prep_multishot_accept(int socket, int flags) {
    // static bool state = false;
    // if (!state) [[unlikely]] {
    //     auto [res, flag] =
    //         co_await MultishotAcceptAwaiter{ net::context::loop.get_uring_loop(), [&](io_uring_sqe* sqe) {
    //                                             io_uring_prep_multishot_accept(sqe, socket, nullptr, nullptr, flags);
    //                                         } };

    //     if (res < 0) [[unlikely]] {
    //         throw std::runtime_error("io_uring prep multishot accept failed.");
    //     }

    //     state = true;
    //     co_return res;
    // }
}

}  // namespace net::io::operation