#pragma once

#include <liburing.h>

#include <tuple>

#include "co_net/async/task.hpp"
#include "co_net/io/prep/caller_coro.hpp"
#include "co_net/io/uring.hpp"

namespace net::io {

template <typename T>
struct UringAwaiter {
public:
    template <typename F>
        requires std::is_invocable_v<F, io_uring_sqe*>
    explicit UringAwaiter(io::Uring& ring, F&& func) {
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

    std::tuple<int, int> await_resume() noexcept { return std::make_tuple(caller_.cqe_res_, caller_.cqe_flags_); }

protected:
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
