#pragma once

#include <span>
#include <string_view>
#include <vector>

#include "co_net/async/task.hpp"
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
    explicit TcpConnection(::net::DirectSocket sock) : socket_(std::move(sock)) {}

    TcpConnection(TcpConnection&& rhs) : socket_(std::move(rhs.socket_)) {}

    TcpConnection& operator=(TcpConnection&& rhs) {
        socket_ = std::move(rhs.socket_);
        return *this;
    }

    ~TcpConnection() = default;

    [[nodiscard]]
    std::string address() const noexcept {
        return socket_.address();
    }

    [[nodiscard]]
    int fd() const noexcept {
        return socket_.fd();
    }

    // ::net::async::Task<void> disconnect() {
    //     auto ret = co_await ::net::io::operation::prep_close_socket(socket_.fd());
    //     if (!ret) {
    //         socket_.release();
    //     }
    // }

    ::net::async::Task<ssize_t> read_some(std::span<char> buf) {
        co_return co_await ::net::io::operation::prep_read(socket_.fd(), buf);
    }

    ::net::async::Task<ssize_t> write_some(std::span<char> buf, size_t len) {
        co_return co_await ::net::io::operation::prep_write(socket_.fd(), buf, len);
    }

    ::net::async::Task<ssize_t> write_all(std::span<char> buf) {
        co_return co_await ::net::io::operation::prep_write(socket_.fd(), buf);
    }

    ::net::async::Task<ssize_t> ring_buf_receive() {
        auto [res, flag] = co_await ::net::io::operation::prep_recv_in_ring_buf(socket_.fd());

        while (res == -ENOBUFS) {
            if constexpr (!config::AUTO_EXTEND_WHEN_NOBUFS) {
                co_await sleep();
            } else {
                this_ctx::local_ring_buffer->extend_buffer();
            }

            auto [res_, flag_] = co_await ::net::io::operation::prep_recv_in_ring_buf(socket_.fd());
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
    ::net::DirectSocket socket_;

    std::vector<char> send_buffer_;

    std::vector<char> receive_buffer_;
};

}  // namespace net::tcp
