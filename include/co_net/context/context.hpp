#pragma once

#include <signal.h>
#include <sys/syscall.h>

#include <latch>
#include <thread>

#include "co_net/async/task.hpp"
#include "co_net/config.hpp"
#include "co_net/context/task_loop.hpp"
#include "co_net/context/worker.hpp"
#include "co_net/dump.hpp"
#include "co_net/io/prep/prep_ring_msg.hpp"
#include "co_net/io/uring.hpp"
#include "co_net/timer/timer.hpp"

namespace net::context {

class SystemLoop : tools::Noncopyable {
private:
    enum SystemLoopStatus { Working, Stopped };

public:
    SystemLoop() :
        main_task_queue_(),
        main_uring_(&main_task_queue_),
        io_buffer_(main_uring_.get()),
        init_latch_(config::WORKERS ? config::WORKERS : std::thread::hardware_concurrency()),
        workers_(config::WORKERS, main_uring_.fd(), init_latch_),
        status_(SystemLoopStatus::Working) {
        init_latch_.wait();
        this_ctx::local_task_queue = &main_task_queue_;
        this_ctx::local_uring_loop = &main_uring_;
        this_ctx::local_ring_buffer = &io_buffer_;
        sys_ctx::sys_task_queue = &main_task_queue_;
        sys_ctx::sys_uring_loop = &main_uring_;

        auto ret = io_uring_register_files_sparse(main_uring_.get(), config::URING_DIRECT_FD_TABLE_SIZE);
        if (ret < 0) {
            throw std::runtime_error("Context register files error");
            std::exit(1);
        }
    }

    ~SystemLoop() { terminate(); }

    void submit_task(std::coroutine_handle<> task) { main_task_queue_.enqueue(task); }

    void run() {
        using namespace net::time_literals;

        int ret{};
        while (status_ == SystemLoopStatus::Working || ret != -ETIME) {
            main_task_queue_.run();

            main_uring_.submit_all();

            ret = main_uring_.wait_cqe_arrival_for(50_ms);

            if (!ret) {
                main_uring_.process_batch();
            }
        }
    }

    [[nodiscard]]
    int pick_worker_uring(int index) {
        return workers_.get_uring_id_by_index(index);
    }

    void terminate() {
        if (status_ == SystemLoopStatus::Working) {
            status_ = SystemLoopStatus::Stopped;
            workers_.stop();
            main_uring_.notify_workers_to_stop(workers_.get_worker_ids());
        }
    }

private:
    ::net::context::TaskLoop main_task_queue_;

    ::net::io::Uring main_uring_;

    ::net::io::RingBuffer io_buffer_;

    ::net::context::Workers workers_;

    std::latch init_latch_;

    SystemLoopStatus status_;
};

void sig_terminate(int signum) {
    Dump(), "SIGINT";
    sys_ctx::sys_loop->terminate();
}

[[nodiscard]]
inline int block_on(async::Task<void>&& task) {
    SystemLoop loop{};
    sys_ctx::sys_loop = &loop;

    struct sigaction when_sigint;
    when_sigint.sa_flags = SA_SIGINFO;
    when_sigint.sa_handler = &sig_terminate;
    sigaction(SIGINT, &when_sigint, nullptr);

    loop.submit_task(task.handle());
    loop.run();
    Dump(), "Terminante";
    return 0;
}

[[nodiscard]]
inline int block_on() {
    SystemLoop loop{};
    sys_ctx::sys_loop = &loop;
    Dump(), "zhe ye xing?";
    return 0;
}

inline async::Task<int> parallel_spawn(async::Task<void>&& task) {
    static int worker = 0;

    // auto thread_cnt = config::WORKERS ? config::WORKERS : std::thread::hardware_concurrency();

    // int current_worker = worker;

    // worker = worker % thread_cnt + 1;

    // co_return co_await net::io::operation::prep_ring_msg(sys_ctx::sys_loop->pick_worker_uring(current_worker),
    //                                                      std::move(task));

    co_return 42;
}

inline void co_spawn(async::Task<void>&& task) {
    auto handle = task.take();
    this_ctx::local_task_queue->enqueue(handle);
}

}  // namespace net::context
