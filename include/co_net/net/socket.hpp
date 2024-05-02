#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "co_net/io/prep/prep_close.hpp"
#include "co_net/io/prep/prep_direct_socket.hpp"
#include "co_net/io/prep/prep_socket.hpp"
#include "co_net/io/uring.hpp"
#include "co_net/net/ip_addr.hpp"

namespace net {

class Socket {
public:
    explicit Socket(int fd, const ip::SocketAddr& addr) : fd_(fd), addr_(addr) { set_reuse(true); }

    explicit Socket(int fd, bool v4) : fd_(fd), addr_(fd, v4) {}

    Socket(Socket&& rhs) noexcept {
        fd_ = rhs.fd_;
        addr_ = rhs.addr_;
        rhs.fd_ = -1;
    }

    Socket& operator=(Socket&& rhs) noexcept {
        fd_ = rhs.fd_;
        addr_ = rhs.addr_;
        rhs.fd_ = -1;
        return *this;
    }

    ~Socket() {
        if (fd_ > -1) {
            ::close(fd_);
        }
        fd_ = -1;
    }

    void set_nonblock(bool on) {
        auto flags = ::fcntl(fd_, F_GETFL, 0);
        if (on) {
            flags |= O_NONBLOCK;
        } else {
            flags &= ~O_NONBLOCK;
        }
        if (::fcntl(fd_, F_SETFL, flags) == -1) [[unlikely]] {
            throw std::runtime_error("Set socket non-block failed.\nNote: Direct Socket Cannot Set Nonblock");
        }
    }

    void set_reuse(bool on) {
        auto optval = on ? 1 : 0;
        if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval, sizeof(optval)) == -1) [[unlikely]] {
            throw std::runtime_error("Set socket resuable failed.\nNote: Direct Socket Cannot Set Reuse");
        }
    }

    [[nodiscard]]
    const int fd() const noexcept {
        return fd_;
    }

    [[nodiscard]]
    int fd() noexcept {
        return fd_;
    }

    [[nodiscard]]
    sockaddr* address() {
        return addr_.is_ipv4() ? reinterpret_cast<sockaddr*>(&addr_.sockaddr_v4())
                               : reinterpret_cast<sockaddr*>(&addr_.sockaddr_v6());
    }

    [[nodiscard]]
    std::string address() const noexcept {
        return addr_.to_string();
    }

    [[nodiscard]]
    socklen_t len() {
        return addr_.len();
    }

    [[nodiscard]]
    bool is_valid() const noexcept {
        return fd_ != -1;
    }

    [[nodiscard]]
    bool is_ipv4() const noexcept {
        return addr_.is_ipv4();
    }

    [[nodiscard]]
    bool is_ipv6() const noexcept {
        return !addr_.is_ipv4();
    }

private:
    int fd_{ -1 };
    net::ip::SocketAddr addr_;
};

class DirectSocket {
public:
    DirectSocket(int fd, bool v4) : fd_(fd), addr_(fd, v4) {}

    DirectSocket(DirectSocket&& rhs) noexcept {
        fd_ = rhs.fd_;
        addr_ = rhs.addr_;
        rhs.fd_ = -1;
    }

    DirectSocket& operator=(DirectSocket&& rhs) noexcept {
        fd_ = rhs.fd_;
        addr_ = rhs.addr_;
        rhs.fd_ = -1;
        return *this;
    }

    ~DirectSocket() {
        if (fd_ > -1) {
            Dump();
            auto* sqe = this_ctx::local_uring_loop->get_sqe();
            io_uring_prep_close_direct(sqe, fd_);
            ::net::io::CompletionToken* token = new ::net::io::CompletionToken{};
            token->op_ = ::net::io::Op::CloseDirect;
            io_uring_sqe_set_data(sqe, &token);
            this_ctx::local_uring_loop->submit_all();
        }
        fd_ = -1;
    }

    ::net::async::Task<void> release_direct(int sock) {
        co_await ::net::io::operation::prep_close_socket_direct(sock);
        fd_ = -1;
    }

    [[nodiscard]]
    const int fd() const noexcept {
        return fd_;
    }

    [[nodiscard]]
    int fd() noexcept {
        return fd_;
    }

    [[nodiscard]]
    sockaddr* address() {
        return addr_.is_ipv4() ? reinterpret_cast<sockaddr*>(&addr_.sockaddr_v4())
                               : reinterpret_cast<sockaddr*>(&addr_.sockaddr_v6());
    }

    [[nodiscard]]
    std::string address() const noexcept {
        return addr_.to_string();
    }

    [[nodiscard]]
    socklen_t len() {
        return addr_.len();
    }

    [[nodiscard]]
    bool is_valid() const noexcept {
        return fd_ != -1;
    }

    [[nodiscard]]
    bool is_ipv4() const noexcept {
        return addr_.is_ipv4();
    }

    [[nodiscard]]
    bool is_ipv6() const noexcept {
        return !addr_.is_ipv4();
    }

private:
    int fd_{ -1 };
    net::ip::SocketAddr addr_;
};

::net::async::Task<Socket> create_tcp_socket(const ::net::ip::SocketAddr& addr) {
    auto socktype = addr.is_ipv4() ? AF_INET : AF_INET6;
    auto sockfd = co_await ::net::io::operation::prep_normal_socket(socktype, SOCK_STREAM, 0, 0);
    co_return Socket{ sockfd, addr };
}

// ! todo udp
::net::async::Task<Socket> create_udp_socket(const ::net::ip::SocketAddr& addr) {
    auto sockfd = co_await ::net::io::operation::prep_normal_socket(AF_INET, SOCK_DGRAM, 0, 0);
}

}  // namespace net
