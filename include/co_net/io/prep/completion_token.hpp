#pragma once

#include <stdint.h>

#include <coroutine>

namespace net::async {
class ScheduledTask;
}

namespace net::io {

enum class Op {
    Wakeup,
    Timeout,
    Socket,
    Accept,
    MultishotAccept,
    RingTask,
    RingFdSender,
    RingFdReceiver,
    Send,
    Receive,
    MultishotReceive,
    RegisterRingBuffer,
    Shutdown,
    SyncCloseDirect,
    AsyncCloseDirect,
    MainRingCloseDirectSocket,
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

}  // namespace net::io