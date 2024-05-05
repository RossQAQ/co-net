#pragma once

#include <signal.h>
#include <sys/syscall.h>

#include <latch>
#include <memory>
#include <thread>
#include <type_traits>

#include "co_net/async/task.hpp"
#include "co_net/config.hpp"
#include "co_net/context/task_loop.hpp"
#include "co_net/context/worker.hpp"
#include "co_net/dump.hpp"
#include "co_net/io/uring.hpp"
#include "co_net/io/uring_msg.hpp"
#include "co_net/net/tcp/connection.hpp"
#include "co_net/timer/timer.hpp"
#include "co_net/traits.hpp"

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

            if (main_task_queue_.empty()) {
                break;
            }

            main_uring_.submit_all();

            ret = main_uring_.wait_cqe_arrival_for(50_ms);

            if (!ret) {
                main_uring_.process_batch();
            }
        }
    }

    void terminate() {
        using namespace std::chrono_literals;

        if (!stopped_) {
            stopped_ = true;
            workers_.stop();
            main_uring_.notify_workers_to_stop(workers_.get_worker_ids());
            main_uring_.notify_self_quit();
            main_uring_.submit_all();
            main_uring_.wait_cqe_arrival();
            // std::this_thread::sleep_for(5s);
            main_uring_.process_all();
        }
    }

    [[nodiscard]]
    int pick_worker() {
        current_worker_uring_idx_ = (current_worker_uring_idx_ + 1) % workers_.count();
        return workers_.get_uring_fd(current_worker_uring_idx_);
    }

private:
    ::net::context::TaskLoop main_task_queue_;

    ::net::io::Uring main_uring_;

    ::net::io::RingBuffer io_buffer_;

    ::net::context::Workers workers_;

    std::latch init_latch_;

    bool stopped_{ false };

    int current_worker_uring_idx_{ -1 };
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

    this_ctx::local_task_queue->emplace_back(std::move(task), std::forward<Args>(args)...);
    loop.run();
    return 0;
}

template <typename VoidTask, typename Connection, typename... Args>
    requires(std::is_same_v<std::remove_cvref_t<Connection>, net::tcp::TcpConnection> &&
             std::is_invocable_r_v<async::Task<void>, VoidTask, Connection, Args...> &&
             std::is_rvalue_reference_v<VoidTask &&>)
inline void parallel_spawn(VoidTask&& task, Connection&& conn, Args&&... args) {
    static_assert(!net::traits::has_direct_fd_v<Args...>,
                  "Currently only support send one TcpConnection to other threads once a time");

    if (sys_ctx::sys_uring_loop->fd() != this_ctx::local_uring_loop->fd()) [[unlikely]] {
        throw std::logic_error("You can only call this function in main thread");
    }

    conn.release();

    int fd = conn.fd();

    auto worker = sys_ctx::sys_loop->pick_worker();

    auto* send_fd = sys_ctx::sys_uring_loop->get_sqe();
    auto* send_task = sys_ctx::sys_uring_loop->get_sqe();

    std::function<async::Task<>(Connection)> func = std::bind(task, std::placeholders::_1, std::forward<Args>(args)...);

    auto* helper = new io::msg::RingMsgConnTaskHelper{ std::move(func), fd };

    auto* sender_token = new io::CompletionToken{};
    sender_token->op_ = io::Op::RingFdSender;
    auto* receiver_token = new io::CompletionToken{};
    receiver_token->op_ = io::Op::RingFdReceiver;
    receiver_token->cqe_res_ = fd;

    io_uring_prep_msg_ring_fd_alloc(send_fd, worker, fd, reinterpret_cast<unsigned long long>(receiver_token), 0);
    io_uring_sqe_set_data(send_fd, static_cast<void*>(sender_token));

    send_fd->flags |= IOSQE_IO_LINK;

    io_uring_prep_msg_ring_cqe_flags(send_task,
                                     worker,
                                     static_cast<uint32_t>(io::msg::RingMsgType::RingFuncTask),
                                     reinterpret_cast<unsigned long long>(helper),
                                     0,
                                     NET_IORING_CQE_F_TASK);

    send_task->flags |= IOSQE_CQE_SKIP_SUCCESS;

    sys_ctx::sys_uring_loop->submit_all();
}

template <typename VoidTask, typename... Args>
    requires(std::is_invocable_r_v<async::Task<void>, VoidTask, Args...> && std::is_rvalue_reference_v<VoidTask &&>)
inline void parallel_spawn(VoidTask&& task, Args&&... args) {
    static_assert(!net::traits::has_direct_fd_v<Args...>,
                  "Use another version of parallel_spawn to send a direct fd to other threads.");

    if (sys_ctx::sys_uring_loop->fd() != this_ctx::local_uring_loop->fd()) [[unlikely]] {
        throw std::logic_error("You can noly call this function in main thread");
    }

    auto* task_msg = new net::io::msg::RingMsgTask(std::invoke(task, std::forward<Args>(args)...));
    auto* token = new io::CompletionToken{};
    token->op_ = io::Op::RingTask;

    auto* send_task_sqe = sys_ctx::sys_uring_loop->get_sqe();

    io_uring_prep_msg_ring_cqe_flags(send_task_sqe,
                                     sys_ctx::sys_loop->pick_worker(),
                                     static_cast<uint32_t>(io::msg::RingMsgType::RingTask),
                                     reinterpret_cast<unsigned long long>(task_msg),
                                     0,
                                     NET_IORING_CQE_F_TASK);

    io_uring_sqe_set_data(send_task_sqe, static_cast<void*>(token));

    sys_ctx::sys_uring_loop->submit_all();
}

template <typename VoidTask, typename... Args>
    requires(std::is_invocable_r_v<async::Task<void>, VoidTask, Args...> && std::is_rvalue_reference_v<VoidTask &&>)
inline void co_spawn(VoidTask&& task, Args&&... args) {
    // ! bug
    // this will heap-use-after-free;
    // currently solution is mark the root chain task passed by block on;
    // will be sloved in future.
    this_ctx::local_task_queue->emplace_back(std::move(task), std::forward<Args>(args)...);
}

}  // namespace net::context

namespace net::io {

void Uring::mshot_accept_send_awaiting_fd_and_process(
    std::function<async::Task<void>(net::tcp::TcpConnection)> func_task) {
    for (auto fd : awaiting_direct_fd_) {
        auto worker = sys_ctx::sys_loop->pick_worker();
        Dump(), worker;

        auto* send_fd = sys_ctx::sys_uring_loop->get_sqe();
        auto* send_task = sys_ctx::sys_uring_loop->get_sqe();
        auto* close_fd = sys_ctx::sys_uring_loop->get_sqe();

        auto* receiver_token = new CompletionToken{};
        receiver_token->op_ = io::Op::MultishotAcceptFdReceiver;

        auto* task_token =
            new io::msg::MultishotAcceptTaskToken{ io::Op::MultishotAcceptTaskReceiver, std::move(func_task) };

        io_uring_prep_msg_ring_fd_alloc(send_fd, worker, fd, reinterpret_cast<unsigned long long>(receiver_token), 0);

        io_uring_prep_msg_ring_cqe_flags(send_task,
                                         worker,
                                         static_cast<uint32_t>(io::msg::RingMsgType::MultiAccTask),
                                         reinterpret_cast<unsigned long long>(task_token),
                                         0,
                                         NET_IORING_CQE_F_TASK);

        io_uring_prep_close_direct(close_fd, fd);

        send_fd->flags |= IOSQE_CQE_SKIP_SUCCESS | IOSQE_IO_LINK;
        send_task->flags |= IOSQE_CQE_SKIP_SUCCESS | IOSQE_IO_LINK;
        close_fd->flags |= IOSQE_CQE_SKIP_SUCCESS;
    }
    awaiting_direct_fd_.clear();
    sys_ctx::sys_uring_loop->submit_all();
}

}  // namespace net::io
