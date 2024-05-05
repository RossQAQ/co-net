#pragma once

#include <liburing.h>
#include <sys/socket.h>

#include "co_net/context/context.hpp"
#include "co_net/io/uring.hpp"

namespace net::io::operation {

class MultishotAcceptDirectAwaiter {
public:
    MultishotAcceptDirectAwaiter(int socket);

public:
    bool await_ready() const noexcept { return false; }

    template <typename Promise>
    void await_suspend(std::coroutine_handle<Promise> caller) {
        MultishotAcceptDirectAwaiter::multishot_token_.chain_task_ = caller.promise().this_chain_task_;
        MultishotAcceptDirectAwaiter::multishot_token_.chain_task_->set_awaiting_handle(caller);
        return;
    }

    void await_resume() noexcept {}

    io::CompletionToken* token_address() { return &multishot_token_; }

public:
    static io::CompletionToken multishot_token_;
};

io::CompletionToken MultishotAcceptDirectAwaiter::multishot_token_;

inline net::async::Task<void> prep_multishot_accept_direct(int socket) {
    co_await MultishotAcceptDirectAwaiter{ socket };
}

}  // namespace net::io::operation