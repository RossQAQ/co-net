#pragma once

#include <stdint.h>

#include <coroutine>

#include "co_net/async/scheduled_task.hpp"

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
    SyncCloseDirect,
    AsyncCloseDirect,
    Close,
    CancelAny,
    Quit,
    Unknown,
};

struct CompletionToken {
    Op op_{ Op::Unknown };
    net::async::ScheduledTask* chain_task_{ nullptr };
    int32_t cqe_res_{ -ENOSYS };
    int32_t cqe_flags_;
};

struct AsyncResult {
    Op op_{ Op::Unknown };
    int32_t cqe_res_{ -ENOSYS };
    int32_t cqe_flags_;
};

}  // namespace net::io