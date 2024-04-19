#pragma once

#include <span>
#include <vector>

#include "co_net/async/task.hpp"
#include "co_net/net/socket.hpp"
#include "co_net/util/noncopyable.hpp"

namespace net::tcp {

class TcpConnection : public ::tools::Noncopyable {
public:
    explicit TcpConnection(::net::Socket sock) : socket_(std::move(sock)) {}

    TcpConnection(TcpConnection&& rhs) : socket_(std::move(rhs.socket_)) {}

    TcpConnection& operator=(TcpConnection&& rhs) {
        socket_ = std::move(rhs.socket_);
        return *this;
    }

    ::net::async::Task<ssize_t> read(std::span<char> buf);

    ::net::async::Task<ssize_t> read();

    ::net::async::Task<ssize_t> write(std::span<char> buf, size_t len);

    ::net::async::Task<ssize_t> write(size_t len);

private:
    ::net::Socket socket_;

    std::vector<char> buffer_;
};

// ::net::aysnc::Task<TcpConnection> connect() {}

}  // namespace net::tcp
