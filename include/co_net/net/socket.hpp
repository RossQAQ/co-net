#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "co_net/io/prep/prep_socket.hpp"
#include "co_net/net/ip_addr.hpp"

namespace net {

class Socket {
public:
    explicit Socket(int fd, const ip::SocketAddr& addr) : fd_(fd), addr_(addr) { set_reuse(true); }

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

    ~Socket() { close(); }

    [[nodiscard]]
    const int fd() const noexcept {
        return fd_;
    }

    [[nodiscard]]
    int fd() noexcept {
        return fd_;
    }

    void close() noexcept {
        if (fd_ > 0) {
            ::close(fd_);
        }
    }

    void set_nonblock(bool on) {
        auto flags = ::fcntl(fd_, F_GETFL, 0);
        if (on) {
            flags |= O_NONBLOCK;
        } else {
            flags &= ~O_NONBLOCK;
        }
        if (::fcntl(fd_, F_SETFL, flags) == -1) [[unlikely]] {
            throw std::runtime_error("Set socket non-block failed.");
        }
    }

    void set_reuse(bool on) {
        auto optval = on ? 1 : 0;
        if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval, sizeof(optval)) == -1) [[unlikely]] {
            throw std::runtime_error("Set socket non-block failed.");
        }
    }

    [[nodiscard]]
    sockaddr* address() {
        return addr_.is_ipv4() ? addr_.sockaddr_v4() : addr_.sockaddr_v6();
    }

    [[nodiscard]]
    socklen_t addr_len() {
        return addr_.len();
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
