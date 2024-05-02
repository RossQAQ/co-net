#pragma once

#include <liburing.h>

#include <array>
#include <cstring>

#include "co_net/async/task.hpp"
#include "co_net/config.hpp"
#include "co_net/context/context.hpp"
#include "co_net/context/task_loop.hpp"
#include "co_net/io/buffer.hpp"
#include "co_net/io/prep/completion_token.hpp"
#include "co_net/io/uring_msg.hpp"
#include "co_net/util/noncopyable.hpp"

/**
 * co_net: cqe-> flags
 * now in io_uring, there are four cqe flags:
 * IORING_CQE_F_BUFFER
 * IORING_CQE_F_MORE
 * IORING_CQE_F_SOCKEMPTY
 * IORING_CQE_F_NOTIFY
 */

// #define IORING_CQE_F_QUIT ~0U

namespace net::io {

class Uring : public tools::Noncopyable {
private:
    enum class UringStatus { Pending, Stopped };

public:
    Uring(net::context::TaskLoop* peer_task_loop) : peer_task_loop_(peer_task_loop) {
        int ret{};
        io_uring_params params;
        std::memset(&params, 0, sizeof(io_uring_params));

        params.cq_entries = config::URING_MAIN_CQES;
        params.flags |= IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_CLAMP;
        params.flags |= IORING_SETUP_CQSIZE;

        ret = io_uring_queue_init_params(config::URING_MAIN_SQES, &ring_, &params);

        if (ret) [[unlikely]] {
            throw std::runtime_error("Main io_uring Init Failed");
        }

        io_uring_register_ring_fd(&ring_);
    }

    Uring(int main_uring_fd, net::context::TaskLoop* peer_task_loop) : peer_task_loop_(peer_task_loop) {
        int ret{};
        io_uring_params params;
        std::memset(&params, 0, sizeof(io_uring_params));

        params.wq_fd = main_uring_fd;
        params.flags |= IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_CLAMP;
        params.flags |= IORING_SETUP_ATTACH_WQ;

        ret = io_uring_queue_init_params(config::URING_WOKERS_SQES, &ring_, &params);

        if (ret) [[unlikely]] {
            throw std::runtime_error("Worker io_uring Init failed");
        }

        io_uring_register_ring_fd(&ring_);
    }

    ~Uring() { io_uring_queue_exit(&ring_); }

    int submit_all() noexcept { return io_uring_submit(&ring_); }

    [[nodiscard]]
    io_uring_sqe* get_sqe() noexcept {
        return io_uring_get_sqe(&ring_);
    }

    int wait_cqe_arrival() noexcept {
        io_uring_cqe* cqe{ nullptr };
        return io_uring_wait_cqe(&ring_, &cqe);
    }

    int wait_cqe_arrival_for(__kernel_timespec period) noexcept {
        io_uring_cqe* cqe{ nullptr };
        return io_uring_wait_cqe_timeout(&ring_, &cqe, &period);
    }

    void process_all() noexcept {
        io_uring_cqe* cqe{ nullptr };
        unsigned head{};
        unsigned i{};
        io_uring_for_each_cqe(&ring_, head, cqe) {
            i++;
            handle_request(cqe);
        }
        consume(i);
    }

    void process_batch() noexcept {
        std::array<io_uring_cqe*, config::URING_PEEK_CQES_BATCH> cqes;
        unsigned completed = io_uring_peek_batch_cqe(&ring_, cqes.data(), config::URING_PEEK_CQES_BATCH);
        for (size_t i{}; i < completed && status_ == UringStatus::Pending; ++i) { handle_request(cqes[i]); }
        consume(completed);

        if (status_ == UringStatus::Stopped) [[unlikely]] {
            consume_all();

            auto* cancel_sqe = get_sqe();
            io_uring_prep_cancel(cancel_sqe, 0, IORING_ASYNC_CANCEL_ANY);
            io_uring_submit(&ring_);

            io_uring_cqe* cqe;
            io_uring_wait_cqe(&ring_, &cqe);

            consume_all();
        }
    }

    [[nodiscard]]
    int fd() const noexcept {
        return ring_.ring_fd;
    }

    [[nodiscard]]
    io_uring* get() {
        return &ring_;
    }

private:
    friend class net::context::SystemLoop;

    void handle_request(io_uring_cqe* cqe) {
        CompletionToken* token = reinterpret_cast<CompletionToken*>(cqe->user_data);

        switch (token->op_) {
            case Op::CloseDirect:
                Dump(), "close direct";
                delete token;
                break;

            case Op::Quit:
                status_ = UringStatus::Stopped;
                delete token;
                break;

            case Op::CancelAny:
                delete token;
                break;

            default:
                token->cqe_res_ = cqe->res;
                token->cqe_flags_ = cqe->flags;
                peer_task_loop_->enqueue(token->handle_);
                break;
        }
    }

    void seen(io_uring_cqe* cqe) noexcept { io_uring_cqe_seen(&ring_, cqe); }

    void consume(size_t cqe_n) noexcept { io_uring_cq_advance(&ring_, cqe_n); }

    void consume_all() noexcept {
        io_uring_cqe* cqe{ nullptr };
        unsigned head{};
        unsigned i{};
        io_uring_for_each_cqe(&ring_, head, cqe) {
            i++;
        }
        consume(i);
    }

    void notify_workers_to_stop(std::span<int> workers) noexcept {
        for (int fd : workers) {
            auto* sqe = get_sqe();
            sqe->flags |= IOSQE_CQE_SKIP_SUCCESS;
            CompletionToken* token = new CompletionToken{};
            token->op_ = Op::Quit;
            io_uring_prep_msg_ring(sqe, fd, 0, reinterpret_cast<unsigned long long>(token), 0);
        }

        notify_self_quit();

        submit_all();
    }

    void notify_self_quit() noexcept {
        auto* sqe = get_sqe();
        sqe->flags |= IOSQE_CQE_SKIP_SUCCESS;
        CompletionToken* token = new CompletionToken{};
        token->op_ = Op::Quit;
        io_uring_prep_msg_ring(sqe, ring_.ring_fd, 0, reinterpret_cast<unsigned long long>(token), 0);
        submit_all();
    }

private:
    io_uring ring_;

    UringStatus status_{ UringStatus::Pending };

    ::net::context::TaskLoop* peer_task_loop_;
};
}  // namespace net::io
