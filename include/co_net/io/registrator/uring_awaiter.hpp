#pragma once

#include <liburing.h>

#include "co_net/async_operation/task.hpp"
#include "co_net/io/registrator/caller_coro.hpp"
#include "co_net/io/uring.hpp"

namespace net::io {

template <typename T>
struct UringAwaiter {
public:
    template <typename F>
        requires std::is_invocable_v<F, io_uring_sqe*>
    explicit UringAwaiter(Uring& ring, F&& func) : ring_(ring) {
        sqe_ = ring.get_sqe();
        func(sqe_);
        io_uring_sqe_set_data(sqe_, &caller_);
    }

    UringAwaiter(const UringAwaiter&) = delete;

    UringAwaiter& operator=(const UringAwaiter&) = delete;

    UringAwaiter(UringAwaiter&&) = delete;

    UringAwaiter& operator=(UringAwaiter&&) = delete;

public:
    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> caller) {
        caller_.handle_ = caller;
        return;
    }

protected:
    Uring& ring_;

    io_uring_sqe* sqe_{ nullptr };

    CallerCoro caller_{ nullptr };
};

// inline net::async::Task<int> uring_prep_socket_direct(io::Uring& ring,
//                                                       int domain,
//                                                       int type,
//                                                       int protocol,
//                                                       unsigned int flags) {
//     co_return co_await net::io::UringAwaiter{
//         ring,
//         [&](io_uring_sqe* sqe) {
//             io_uring_prep_socket_direct(sqe, domain, type, protocol, IORING_FILE_INDEX_ALLOC, flags);
//         }
//     };
// }

}  // namespace net::io
