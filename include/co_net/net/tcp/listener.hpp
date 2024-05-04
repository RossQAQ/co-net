#pragma once

#include <sys/socket.h>

#include <map>
#include <tuple>
#include <vector>

#include "co_net/io/prep/prep_accept.hpp"
#include "co_net/io/prep/prep_socket.hpp"
#include "co_net/net/ip_addr.hpp"
#include "co_net/net/socket.hpp"
#include "co_net/net/tcp/connection.hpp"
#include "co_net/util/noncopyable.hpp"

namespace net::tcp {

class TcpListener : public ::tools::Noncopyable {
public:
    TcpListener(::net::Socket&& socket) : socket_(std::move(socket)) {}

    TcpListener(TcpListener&& rhs) : socket_(std::move(rhs.socket_)) {}

    TcpListener& operator=(TcpListener&& rhs) {
        socket_ = std::move(rhs.socket_);
        return *this;
    }

    const int listen_fd() const noexcept { return socket_.fd(); }

    ::net::async::Task<TcpConnection> accept() {
        int client_sock = co_await ::net::io::operation::prep_accept_direct(listen_fd());

        if (socket_.is_ipv4()) {
            co_return TcpConnection{ client_sock };
        } else {
            co_return TcpConnection{ client_sock };
        }
    }

    ::net::async::Task<TcpConnection> multishot_accept();

public:
    static ::net::async::Task<TcpListener> listen_on(::net::ip::SocketAddr addr, int backlog = SOMAXCONN) {
        auto socket = co_await ::net::create_tcp_socket(addr);
        auto listener = TcpListener{ std::move(socket) };
        listener.bind();
        listener.listen(backlog);
        co_return listener;
    }

private:
    void bind() { ::bind(socket_.fd(), socket_.address(), socket_.len()); }

    void listen(int backlog) { ::listen(socket_.fd(), backlog); }

private:
    ::net::Socket socket_;

    // ring_fd, direct socket;
    std::map<int, std::vector<int>> connections_;
};

}  // namespace net::tcp
