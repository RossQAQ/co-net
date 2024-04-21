#pragma once

#include <latch>
#include <thread>

#include "co_net/async/task.hpp"
#include "co_net/config.hpp"
#include "co_net/context/task_loop.hpp"
#include "co_net/context/worker.hpp"
#include "co_net/dump.hpp"
#include "co_net/io/prep/prep_ring_msg.hpp"
#include "co_net/io/uring.hpp"

namespace net::context {

class SystemLoop : tools::Noncopyable {
public:
    SystemLoop(net::io::UringType uring_type) :
        main_task_queue_(),
        main_uring_(uring_type, &main_task_queue_),
        init_latch_(config::WORKERS ? config::WORKERS : std::thread::hardware_concurrency()),
        workers_(config::WORKERS, main_uring_.fd(), init_latch_) {
        init_latch_.wait();
        this_ctx::local_task_queue = &main_task_queue_;
        this_ctx::local_uring_loop = &main_uring_;
        sys_ctx::sys_task_queue = &main_task_queue_;
        sys_ctx::sys_uring_loop = &main_uring_;
    }

    void submit_task(std::coroutine_handle<> task) { main_task_queue_.enqueue(task); }

    void run() {
        for (;;) {
            main_task_queue_.run();
            main_uring_.submit_all();
            main_uring_.wait_cqe_arrival();
            main_uring_.process_batch();
        }
    }

    [[nodiscard]]
    int pick_worker_uring(int index) {
        return workers_.get_uring_id_by_index(index);
    }

private:
    ::net::context::TaskLoop main_task_queue_;

    ::net::io::Uring main_uring_;

    std::latch init_latch_;

    ::net::context::Workers workers_;
};

[[nodiscard]]
inline int block_on(async::Task<void>&& task) {
    static SystemLoop loop{ net::io::UringType::Major };
    sys_ctx::sys_loop = &loop;
    loop.submit_task(task.handle());
    loop.run();
    return 0;
}

inline async::Task<int> parallel_spawn(async::Task<void>&& task) {
    static int worker = 1;

    auto thread_cnt = config::WORKERS ? config::WORKERS : std::thread::hardware_concurrency();

    int current_worker = worker;

    worker = worker % thread_cnt + 1;

    co_return co_await net::io::operation::prep_ring_msg(sys_ctx::sys_loop->pick_worker_uring(current_worker),
                                                         std::move(task));
}

inline void co_spawn(async::Task<void>&& task) {
    auto handle = task.take();
    this_ctx::local_task_queue->enqueue(handle);
}

}  // namespace net::context
