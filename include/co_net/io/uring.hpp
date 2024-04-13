#pragma once

#include <liburing.h>

#include <array>
#include <cstring>

#include "co_net/async/task.hpp"
#include "co_net/config.hpp"
#include "co_net/context/task_loop.hpp"
#include "co_net/io/prep/caller_coro.hpp"
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
        if (ret) [[unlikely]] {
            throw std::runtime_error("io_uring Init failed");
        }

        io_uring_register_ring_fd(&ring_);
    }

    ~Uring() { io_uring_queue_exit(&ring_); }

    int submit_all() noexcept { return io_uring_submit(&ring_); }

    [[nodiscard]]
    io_uring_sqe* get_sqe() noexcept {
        return io_uring_get_sqe(&ring_);
    }

    void wait_one_cqe() noexcept {
        io_uring_cqe* cqe{ nullptr };
        io_uring_wait_cqe(&ring_, &cqe);
        check_current_task(cqe);
        seen(cqe);
    }

    void process_all() noexcept {
        io_uring_cqe* cqe{ nullptr };
        unsigned head{};
        unsigned i{};
        io_uring_for_each_cqe(&ring_, head, cqe) {
            i++;
            check_current_task(cqe);
        }
        consume(i);
    }

    void process_batch() noexcept {
        std::array<io_uring_cqe*, config::URING_PEEK_CQES_BATCH> cqes;
        unsigned completed = io_uring_peek_batch_cqe(&ring_, cqes.data(), config::URING_PEEK_CQES_BATCH);
        for (size_t i{}; i < completed; ++i) { check_current_task(cqes[i]); }
        consume(completed);
    }

    [[nodiscard]]
    int fd() const noexcept {
        return ring_.ring_fd;
    }

private:
    void check_current_task(io_uring_cqe* cqe) {
        auto* caller = reinterpret_cast<CallerCoro*>(cqe->user_data);
        caller->cqe_res_ = cqe->res;
        caller->cqe_flags_ = cqe->flags;
        if (!caller->handle_.done()) {
            peer_task_loop_->enqueue(caller->handle_);
        }
    }

    void seen(io_uring_cqe* cqe) noexcept { io_uring_cqe_seen(&ring_, cqe); }

    void consume(size_t cqe_n) noexcept { io_uring_cq_advance(&ring_, cqe_n); }

private:
    io_uring ring_;

    ::net::context::TaskLoop* peer_task_loop_;
};
}  // namespace net::io
