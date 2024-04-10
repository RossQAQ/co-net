/**
 * @file uring.hpp
 * @author Roseveknif (rossqaq@outlook.com)
 * @brief 提供 io_uring 的抽象
 * @version 0.1
 * @date 2024-03-29
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <liburing.h>

#include <cstring>

#include "co_net/async_operation/task.hpp"
#include "co_net/config.hpp"
#include "co_net/context/task_loop.hpp"
#include "co_net/io/registrator/caller_coro.hpp"
#include "co_net/util/noncopyable.hpp"

namespace net::io {

enum class UringType { Major, Minor };

class Uring : public tools::Noncopyable {
public:
    Uring(UringType type, net::context::TaskLoop* peer_task_loop) : peer_task_loop_(peer_task_loop) {
        int ret{};
        if (type == UringType::Major) {
            io_uring_params params;
            std::memset(&params, 0, sizeof(io_uring_params));
            params.cq_entries = config::URING_MAIN_CQES;
            params.flags |= IORING_SETUP_CQSIZE;
            ret = io_uring_queue_init_params(config::URING_MAIN_SQES, &ring_, &params);
        } else {
            ret = io_uring_queue_init(config::URING_WOKERS_SQES, &ring_, 0);
        }
        if (ret) {
            throw std::runtime_error("io_uring Init failed");
        }
    }

    ~Uring() { io_uring_queue_exit(&ring_); }

    [[nodiscard]]
    int submit_all() noexcept {
        return io_uring_submit(&ring_);
    }

    [[nodiscard]]
    io_uring_sqe* get_sqe() noexcept {
        return io_uring_get_sqe(&ring_);
    }

    void wait_at_least_one_cqe() noexcept {
        io_uring_cqe* cqe{ nullptr };
        Dump(), "Waiting CQE";
        io_uring_wait_cqe(&ring_, &cqe);
        Dump(), "CQE Getted";
        auto* caller = reinterpret_cast<CallerCoro*>(cqe->user_data);
        peer_task_loop_->enqueue(caller->handle_);
        io_uring_cqe_seen(&ring_, cqe);
    }

    void batch_process(size_t num) noexcept {}

    void seen(io_uring_cqe* cqe) noexcept { io_uring_cqe_seen(&ring_, cqe); }

    void consume(size_t cqe_n) noexcept { io_uring_cq_advance(&ring_, cqe_n); }

private:
    io_uring ring_;

    ::net::context::TaskLoop* peer_task_loop_;
};
}  // namespace net::io
