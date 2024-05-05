#pragma once

#include <liburing.h>

#include <array>
#include <cstring>
#include <unordered_map>

#include "co_net/async/task.hpp"
#include "co_net/config.hpp"
#include "co_net/context/context.hpp"
#include "co_net/context/task_loop.hpp"
#include "co_net/io/buffer.hpp"
#include "co_net/io/prep/completion_token.hpp"
#include "co_net/io/prep/prep_multishot_accept.hpp"
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

#define NET_IORING_CQE_F_TASK (1u << 4)
#define NET_IORING_CQE_F_FD   (1u << 5)

namespace net::tcp {

class TcpListener;

}

namespace net::io {

class Uring : public tools::Noncopyable {
public:
    Uring(net::context::TaskLoop* peer_task_loop) : peer_task_loop_(peer_task_loop) {
        awaiting_direct_fd_.reserve(config::URING_MAIN_CQES);

        int ret{};
        io_uring_params params;
        std::memset(&params, 0, sizeof(io_uring_params));

        params.cq_entries = config::URING_MAIN_CQES;
        params.flags |= IORING_SETUP_SINGLE_ISSUER | IORING_SETUP_CLAMP;
        params.flags |= IORING_SETUP_COOP_TASKRUN;
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
        params.flags |= IORING_SETUP_COOP_TASKRUN;
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
        for (size_t i{}; i < completed && !stopped_; ++i) { handle_request(cqes[i]); }
        consume(completed);

        if (stopped_) [[unlikely]] {
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

    void notify_main_ring_release_fd(int direct_fd) {
        CompletionToken* token = new CompletionToken{};
        token->op_ = Op::MainRingCloseDirectSocket;

        auto* notify = get_sqe();
        io_uring_prep_msg_ring(notify,
                               sys_ctx::sys_uring_loop->fd(),
                               direct_fd,
                               reinterpret_cast<unsigned long long>(token),
                               0);
        notify->flags |= IOSQE_CQE_SKIP_SUCCESS;
        submit_all();
    }

private:
    friend class net::context::SystemLoop;

    friend class net::tcp::TcpConnection;

    friend class net::tcp::TcpListener;

    void handle_request(io_uring_cqe* cqe) {
        if ((cqe->res == static_cast<int32_t>(msg::RingMsgType::RingTask)) && (cqe->flags & NET_IORING_CQE_F_TASK))
            [[unlikely]] {
            auto* msg_task = reinterpret_cast<msg::RingMsgTask*>(cqe->user_data);
            peer_task_loop_->enqueue(msg_task->move_out());
            delete msg_task;
        } else if ((cqe->res == static_cast<int32_t>(msg::RingMsgType::RingFuncTask)) &&
                   (cqe->flags & NET_IORING_CQE_F_TASK)) [[unlikely]] {
            impl_handle_msg_conn(cqe);
        } else if ((cqe->res == static_cast<int32_t>(msg::RingMsgType::MultiAccTask)) &&
                   (cqe->flags & NET_IORING_CQE_F_TASK)) [[unlikely]] {
            impl_handle_macc_task(cqe);
        } else {
            CompletionToken* token = reinterpret_cast<CompletionToken*>(cqe->user_data);
            if (token) [[likely]] {
                switch (token->op_) {
                    case Op::SyncCloseDirect:
                        delete token;
                        break;

                    case Op::Quit:
                        stopped_ = true;
                        delete token;
                        break;

                    case Op::CancelAny:
                        delete token;
                        break;

                    case Op::RingTask:
                        delete token;
                        break;

                    case Op::RingFdSender:
                        delete token;
                        break;

                    case Op::RingFdReceiver:
                        direct_map_[token->cqe_res_] = cqe->res;
                        delete token;
                        break;

                    case Op::MainRingCloseDirectSocket:
                        close_direct_socket(cqe->res);
                        delete token;
                        break;

                    case Op::MultishotAccept:
                        if (cqe->res >= 0) [[likely]] {
                            awaiting_direct_fd_.push_back(cqe->res);
                        }
                        io::operation::MultishotAcceptDirectAwaiter::multishot_token_.chain_task_->set_pending(false);
                        break;

                    case Op::MultishotAcceptFdReceiver:
                        if (cqe->res >= 0) [[likely]] {
                            awaiting_direct_fd_.push_back(cqe->res);
                        }
                        delete token;
                        break;

                    default:
                        token->cqe_res_ = cqe->res;
                        token->cqe_flags_ = cqe->flags;
                        token->chain_task_->set_pending(false);
                        break;
                }
            }
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
    }

    void notify_self_quit() noexcept {
        auto* sqe = get_sqe();
        sqe->flags |= IOSQE_CQE_SKIP_SUCCESS;
        CompletionToken* token = new CompletionToken{};
        token->op_ = Op::Quit;
        io_uring_prep_msg_ring(sqe, ring_.ring_fd, 0, reinterpret_cast<unsigned long long>(token), 0);
    }

    void impl_handle_msg_conn(io_uring_cqe* cqe);

    void impl_handle_macc_task(io_uring_cqe* cqe);

    void close_direct_socket(int direct_fd) {
        auto* sqe = this->get_sqe();
        io_uring_prep_close_direct(sqe, direct_fd);
        sqe->flags |= IOSQE_CQE_SKIP_SUCCESS;
        submit_all();
    }

    void mshot_accept_send_awaiting_fd_and_process(std::function<async::Task<void>(net::tcp::TcpConnection)> func_task);

private:
    io_uring ring_;

    bool stopped_{ false };

    std::unordered_map<int, int> direct_map_;

    // for main ring: this is used to send all accept fd;
    // for worker ring: this is used to store the awating direct fd in mshot accept;
    std::vector<int> awaiting_direct_fd_;

    ::net::context::TaskLoop* peer_task_loop_;
};

}  // namespace net::io

namespace net::io::operation {

MultishotAcceptDirectAwaiter::MultishotAcceptDirectAwaiter(int socket) {
    auto* sqe = sys_ctx::sys_uring_loop->get_sqe();

    io_uring_prep_multishot_accept_direct(sqe, socket, nullptr, nullptr, 0);

    multishot_token_.op_ = Op::MultishotAccept;
    io_uring_sqe_set_data(sqe, static_cast<void*>(token_address()));
}
}  // namespace net::io::operation

namespace net::impl {};  // namespace net::impl