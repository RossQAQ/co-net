#pragma once

#include <liburing.h>

#include <functional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "co_net/async/task.hpp"
#include "co_net/context/context.hpp"
#include "co_net/io/prep/prep_close.hpp"
#include "co_net/io/prep/prep_read.hpp"
#include "co_net/io/prep/prep_recv_in_ring_buf.hpp"
#include "co_net/io/prep/prep_timeout.hpp"
#include "co_net/io/prep/prep_write.hpp"
#include "co_net/net/socket.hpp"
#include "co_net/util/noncopyable.hpp"

namespace net::tcp {

class TcpConnection : public ::tools::Noncopyable {
public:
    explicit TcpConnection(int direct_socket) : socket_(direct_socket), direct_fd_in_sys_ring_(direct_socket) {}

    TcpConnection(TcpConnection&& rhs) :
        socket_(std::exchange(rhs.socket_, -1)),
        direct_fd_in_sys_ring_(std::exchange(rhs.direct_fd_in_sys_ring_, -1)),
        direct_fd_in_this_ring_(std::exchange(rhs.direct_fd_in_this_ring_, -1)),
        send_buffer_(rhs.move_out_sended()),
        receive_buffer_(rhs.move_out_received()),
        ownership_in_sys_ring_(rhs.ownership_in_sys_ring_) {}

    TcpConnection& operator=(TcpConnection&& rhs) {
        socket_ = std::exchange(rhs.socket_, -1);
        direct_fd_in_sys_ring_ = std::exchange(rhs.direct_fd_in_sys_ring_, -1);
        direct_fd_in_this_ring_ = std::exchange(rhs.direct_fd_in_this_ring_, -1);
        send_buffer_ = rhs.move_out_sended();
        receive_buffer_ = rhs.move_out_received();
        ownership_in_sys_ring_ = rhs.ownership_in_sys_ring_;
        return *this;
    }

    ~TcpConnection() {
        if (ownership_in_sys_ring_) {
            sys_ctx::sys_uring_loop->close_direct_socket(socket_);
            direct_fd_in_sys_ring_ = -1;
        } else {
            if (direct_fd_in_this_ring_ > -1) [[likely]] {
                this_ctx::local_uring_loop->close_direct_socket(socket_);
            }
            direct_fd_in_this_ring_ = -1;
        }
        socket_ = -1;
    }

    void release() {
        if (ownership_in_sys_ring_ == false) {
            throw std::logic_error("You cannot release the connection onwership in worker ring loop.");
        }
        direct_fd_in_sys_ring_ = -1;
        ownership_in_sys_ring_ = false;
    }

    void set_subring_socket(int subring_direct_socket) {
        socket_ = subring_direct_socket;
        direct_fd_in_this_ring_ = subring_direct_socket;
        ownership_in_sys_ring_ = false;
    }

    [[nodiscard]]
    int fd() const noexcept {
        return socket_;
    }

    ::net::async::Task<ssize_t> read_some(std::span<char> buf) {
        co_return co_await ::net::io::operation::prep_read(socket_, buf);
    }

    ::net::async::Task<ssize_t> write_some(std::span<char> buf, size_t len) {
        co_return co_await ::net::io::operation::prep_write(socket_, buf, len);
    }

    ::net::async::Task<ssize_t> write_all(std::span<char> buf) {
        co_return co_await ::net::io::operation::prep_write(socket_, buf);
    }

    ::net::async::Task<ssize_t> ring_buf_receive() {
        auto [res, flag] = co_await ::net::io::operation::prep_recv_in_ring_buf(socket_);

        while (res == -ENOBUFS) {
            if constexpr (!config::AUTO_EXTEND_WHEN_NOBUFS) {
                co_await sleep();
            } else {
                this_ctx::local_ring_buffer->extend_buffer();
            }

            auto [res_, flag_] = co_await ::net::io::operation::prep_recv_in_ring_buf(socket_);
            res = res_;
            flag = flag_;
        }

        int bid = flag >> IORING_CQE_BUFFER_SHIFT;

        receive_buffer_.clear();

        this_ctx::local_ring_buffer->copy_data(receive_buffer_, bid, res);

        this_ctx::local_ring_buffer->repay(bid);

        co_return res;
    }

    ::net::async::Task<ssize_t> ring_buf_multishot_receive();

    // ! todo
    // ::net::async::Task<ssize_t> ring_buffer_read_and_split(std::string_view split) {
    //     auto [res, flag] = co_await ::net::io::operation::prep_recv_in_ring_buf(socket_.fd());

    //     if (res == -ENOBUFS) {
    //         Dump(), strerror(-res);
    //         std::exit(1);
    //         // should solve no buffer situation.
    //     }

    //     int bid = flag >> IORING_CQE_BUFFER_SHIFT;

    //     receive_buffer_.clear();

    //     this_ctx::local_ring_buffer->copy_data(receive_buffer_, bid, res);

    //     this_ctx::local_ring_buffer->repay(bid);

    //     co_return res;
    // }

    ::net::async::Task<void> sleep(__kernel_timespec tm = config::NOBUFS_SLEEP_FOR) {
        co_await ::net::io::operation::async_sleep_for(tm);
    }

    std::vector<char> move_out_sended() { return std::move(send_buffer_); }

    std::vector<char> copy_sended() const noexcept { return send_buffer_; }

    std::vector<char> move_out_received() { return std::move(receive_buffer_); }

    std::vector<char> copy_received() const noexcept { return receive_buffer_; }

private:
    // ::net::DirectSocket socket_;
    int socket_{ -1 };

    int direct_fd_in_sys_ring_{ -1 };

    int direct_fd_in_this_ring_{ -1 };

    bool ownership_in_sys_ring_{ true };

    std::vector<char> send_buffer_;

    std::vector<char> receive_buffer_;
};

}  // namespace net::tcp

namespace net::io {

void Uring::impl_handle_msg_conn(io_uring_cqe* cqe) {
    auto* task_token = reinterpret_cast<msg::RingMsgConnTaskHelper*>(cqe->user_data);
    auto task = task_token->move_out();
    auto sys_conn_fd = task_token->direct_fd_;
    delete task_token;

    auto connfd = direct_map_[sys_conn_fd];
    direct_map_.erase(sys_conn_fd);

    auto conn = net::tcp::TcpConnection{ -1 };
    conn.set_subring_socket(connfd);
    peer_task_loop_->emplace_back(std::move(task), std::move(conn));

    notify_main_ring_release_fd(sys_conn_fd);
}

void Uring::impl_handle_macc_task(io_uring_cqe* cqe) {
    Dump(), "1234";
    auto* task_token = reinterpret_cast<msg::MultishotAcceptTaskToken*>(cqe->user_data);
    auto func = task_token->move_out();
    delete task_token;

    for (auto fd : awaiting_direct_fd_) {
        auto task = std::invoke(func, net::tcp::TcpConnection{ fd });
        peer_task_loop_->enqueue(std::move(task));
    }
}

}  // namespace net::io