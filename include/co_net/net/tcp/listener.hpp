#pragma once

#include <sys/socket.h>

#include <tuple>

#include "co_net/io/prep/prep_socket.hpp"
#include "co_net/net/ip_addr.hpp"
#include "co_net/net/socket.hpp"
#include "co_net/util/noncopyable.hpp"

namespace net::tcp {

class Listener : public ::tools::Noncopyable {
public:
    Listener(::net::Socket&& socket) : socket_(std::move(socket)) {}

    Listener(Listener&& rhs) : socket_(std::move(rhs.socket_)) {}

    Listener& operator=(Listener&& rhs) {
        socket_ = std::move(rhs.socket_);
        return *this;
    }

    void accept();

    void multishot_accept();

    void bind() { ::bind(socket_.fd(), socket_.address(), socket_.addr_len()); }

    void listen(int backlog) { ::listen(socket_.fd(), backlog); }

    const int listen_fd() const noexcept { return socket_.fd(); }

private:
    ::net::Socket socket_;
};

::net::async::Task<Listener> listen_on(::net::ip::SocketAddr addr, int backlog = SOMAXCONN) {
    auto socket = co_await ::net::create_tcp_socket(addr);
    auto listener = Listener{ std::move(socket) };
    listener.bind();
    listener.listen(backlog);
    co_return listener;
}

}  // namespace net::tcp
