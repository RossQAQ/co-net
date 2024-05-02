#pragma once

#include <stdint.h>

#include <coroutine>

namespace net::io {

enum class Op {
    Wakeup,
    Timeout,
    Socket,
    Accept,
    MultishotAccept,
    RingMessage,
    RingFd,
    Send,
    Receive,
    MultishotReceive,
    RegisterRingBuffer,
    Shutdown,
    CloseDirect,
    Close,
    CancelAny,
    Quit,
    Unknown,
};

struct CompletionToken {
    Op op_{ Op::Unknown };
    std::coroutine_handle<> handle_{ nullptr };
    int32_t cqe_res_{ -ENOSYS };
    int32_t cqe_flags_;
};

struct AsyncResult {
    Op op_{ Op::Unknown };
    int32_t cqe_res_{ -ENOSYS };
    int32_t cqe_flags_;
};

}  // namespace net::io