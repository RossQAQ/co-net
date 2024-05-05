#pragma once

#include <sys/socket.h>

#include <map>
#include <tuple>
#include <vector>

#include "co_net/io/prep/prep_accept.hpp"
#include "co_net/io/prep/prep_multishot_accept.hpp"
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

    template <typename VoidTask, typename... Args>
        requires(std::is_invocable_r_v<async::Task<void>, VoidTask, ::net::tcp::TcpConnection, Args...> &&
                 std::is_rvalue_reference_v<VoidTask &&>)
    async::Task<> multishot_accept_then(VoidTask&& task, Args&&... args) {
        co_await ::net::io::operation::prep_multishot_accept_direct(socket_.fd());

        std::function<async::Task<>(::net::tcp::TcpConnection)> func =
            std::bind(task, std::placeholders::_1, std::forward<Args>(args)...);

        for (;;) {
            sys_ctx::sys_uring_loop->mshot_accept_send_awaiting_fd_and_process(func);
            co_await ::net::io::PendingAwaiter{};
        }
    }

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
