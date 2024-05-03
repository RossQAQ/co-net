#pragma once

#include <signal.h>
#include <sys/syscall.h>

#include <latch>
#include <thread>
#include <type_traits>

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
public:
    SystemLoop() :
        main_task_queue_(),
        main_uring_(&main_task_queue_),
        io_buffer_(main_uring_.get()),
        init_latch_(config::WORKERS ? config::WORKERS : std::thread::hardware_concurrency()),
        workers_(config::WORKERS, main_uring_.fd(), init_latch_) {
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

    void run() {
        using namespace net::time_literals;

        int ret{};
        while (!stopped_) {
            main_task_queue_.run();

            main_uring_.submit_all();

            if (main_task_queue_.empty()) {
                break;
            }

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
        using namespace net::time_literals;

        if (!stopped_) {
            stopped_ = true;
            workers_.stop();
            main_uring_.notify_workers_to_stop(workers_.get_worker_ids());
            main_uring_.notify_self_quit();
            main_uring_.submit_all();
            main_uring_.wait_cqe_arrival();
            main_uring_.process_all();
        }
    }

private:
    ::net::context::TaskLoop main_task_queue_;

    ::net::io::Uring main_uring_;

    ::net::io::RingBuffer io_buffer_;

    ::net::context::Workers workers_;

    std::latch init_latch_;

    bool stopped_{ false };
};

template <typename VoidTask, typename... Args>
    requires(std::is_invocable_r_v<async::Task<void>, VoidTask, Args...> && std::is_rvalue_reference_v<VoidTask &&>)
inline int block_on(VoidTask&& task, Args&&... args) {
    SystemLoop loop{};
    sys_ctx::sys_loop = &loop;

    struct sigaction when_sigint;
    when_sigint.sa_flags = SA_SIGINFO;
    when_sigint.sa_handler = [](int signum) {
        Dump(), "SIGINT";
        sys_ctx::sys_loop->terminate();
    };
    sigaction(SIGINT, &when_sigint, nullptr);

    this_ctx::local_task_queue->emplace_back(std::forward<VoidTask>(task), std::forward<Args>(args)...);
    loop.run();
    return 0;
}

template <typename TaskType>
    requires(std::is_same_v<TaskType, net::async::Task<void>> && std::is_rvalue_reference_v<TaskType &&>)
inline void parallel_spawn(TaskType&& task) {
    static int worker = 0;
    Dump(), typeid(TaskType&&).name();

    auto thread_cnt = config::WORKERS ? config::WORKERS : std::thread::hardware_concurrency();

    // int current_worker = worker;

    // worker = worker % thread_cnt + 1;

    // co_return co_await net::io::operation::prep_ring_msg(sys_ctx::sys_loop->pick_worker_uring(current_worker),
    //                                                      std::move(task));

    return;
}

template <typename TaskType, typename... Args>
    requires(std::is_same_v<TaskType, net::async::Task<void>> && std::is_rvalue_reference_v<TaskType &&>)
inline void parallel_spawn(TaskType&& task, int fd, Args&&... args) {
    static int worker = 0;

    auto thread_cnt = config::WORKERS ? config::WORKERS : std::thread::hardware_concurrency();

    // int current_worker = worker;

    // worker = worker % thread_cnt + 1;

    // co_return co_await net::io::operation::prep_ring_msg(sys_ctx::sys_loop->pick_worker_uring(current_worker),
    //                                                      std::move(task));

    return;
}

template <typename VoidTask, typename... Args>
    requires(std::is_invocable_r_v<async::Task<void>, VoidTask, Args...> && std::is_rvalue_reference_v<VoidTask &&>)
inline void co_spawn(VoidTask&& task, Args&&... args) {
    this_ctx::local_task_queue->emplace_back(std::forward<VoidTask>(task), std::forward<Args>(args)...);
}

}  // namespace net::context
