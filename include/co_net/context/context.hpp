#pragma once

#include <thread>

#include "co_net/async/task.hpp"
#include "co_net/config.hpp"
#include "co_net/context/task_loop.hpp"
#include "co_net/context/this_ctx.hpp"
#include "co_net/context/worker.hpp"
#include "co_net/dump.hpp"
#include "co_net/io/uring.hpp"

namespace net::context {

class SystemLoop : tools::Noncopyable {
public:
    SystemLoop(net::io::UringType uring_type) : main_task_queue_(), main_uring_(uring_type, &main_task_queue_), workers_(config::WORKERS) {
        this_ctx::local_task_queue = &main_task_queue_;
        this_ctx::local_uring_loop = &main_uring_;
        system::sys_task_queue = &main_task_queue_;
        system::sys_uring_loop = &main_uring_;
    }

    void submit_task(std::coroutine_handle<> task) { main_task_queue_.enqueue(task); }

    void run() {
        for (;;) {
            main_task_queue_.run();
            main_uring_.submit_all();
            main_uring_.wait_cqe_arrival();
        }
    }

private:
    ::net::context::TaskLoop main_task_queue_;

    ::net::io::Uring main_uring_;

    ::net::context::Workers workers_;
};

inline void co_spawn(async::Task<void>&& task) {
    auto handle = task.take();
    this_ctx::local_task_queue->enqueue(handle);
}

[[nodiscard]]
inline int block_on(async::Task<void>&& task) {
    static SystemLoop loop{ net::io::UringType::Major };
    loop.submit_task(task.handle());
    loop.run();
    return 0;
}

}  // namespace net::context
