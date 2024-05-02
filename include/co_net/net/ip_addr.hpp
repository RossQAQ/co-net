#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

namespace net::ip {

class Ipv4Addr {
public:
    explicit Ipv4Addr(::in_addr v4) noexcept : ip_(v4) {}

    explicit Ipv4Addr(::in_addr_t v4) noexcept { ip_.s_addr = v4; }

    [[nodiscard]]
    const auto addr() const noexcept {
        return ip_;
    }

    [[nodiscard]]
    uint32_t to_u32_network() const noexcept {
        return ip_.s_addr;
    }

    [[nodiscard]]
    uint32_t to_u32_host() const noexcept {
        return ::ntohl(ip_.s_addr);
    }

    [[nodiscard]]
    std::string to_string() const {
        char buf[16];
        if (::inet_ntop(AF_INET, &(ip_.s_addr), buf, INET_ADDRSTRLEN) == nullptr) [[unlikely]] {
            throw std::runtime_error("Ip address(v4) to string failed.");
        }
        return buf;
    }

public:
    static Ipv4Addr parse(std::string_view v4) {
        in_addr_t addr;
        if (::inet_pton(AF_INET, v4.data(), &addr) == -1) [[unlikely]] {
            throw std::runtime_error("IP address(v4) parse failed.");
        }
        return Ipv4Addr{ addr };
    }

private:
    ::in_addr ip_{};
};

class Ipv6Addr {
public:
    explicit Ipv6Addr(::in6_addr v6) noexcept : ip_(v6) {}

    [[nodiscard]]
    const auto addr() const noexcept {
        return ip_;
    }

    [[nodiscard]]
    std::string to_string() const {
        char buf[64];
        if (::inet_ntop(AF_INET6, &ip_, buf, INET6_ADDRSTRLEN) == nullptr) [[unlikely]] {
            throw std::runtime_error("Ip address(v6) to string failed.");
        }
        return buf;
    }

public:
    static Ipv6Addr parse(std::string_view v6) {
        in6_addr addr;
        if (::inet_pton(AF_INET6, v6.data(), &addr) == -1) [[unlikely]] {
            throw std::runtime_error("IP address(v6) parse failed.");
        }
        return Ipv6Addr{ addr };
    }

private:
    ::in6_addr ip_{};
};

class SocketAddr {
public:
    SocketAddr() = default;

    explicit SocketAddr(int fd, bool is_v4) : is_v4_(is_v4) {
        if (is_v4_) {
            sockaddr_in addr{};
            addr_v4_ = addr;
            ::getpeername(fd, reinterpret_cast<sockaddr*>(&addr_v4_), &len_);
        } else {
            sockaddr_in6 addr{};
            addr_v6_ = addr;
            ::getpeername(fd, reinterpret_cast<sockaddr*>(&addr_v6_), &len_);
        }
    }

    SocketAddr(const Ipv4Addr& ipv4, uint16_t port) : is_v4_(true), len_(sizeof(sockaddr_in)) {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr = ipv4.addr();
        addr.sin_port = ::htons(port);
        addr_v4_ = addr;
    }

    SocketAddr(const Ipv6Addr& ipv6, uint16_t port) : is_v4_(false), len_(sizeof(sockaddr_in6)) {
        sockaddr_in6 addr;
        addr.sin6_family = AF_INET6;
        addr.sin6_addr = ipv6.addr();
        addr.sin6_port = ::htons(port);
        addr_v6_ = addr;
    }

    [[nodiscard]]
    bool is_ipv4() const noexcept {
        return is_v4_;
    }

    [[nodiscard]]
    bool is_ipv6() const noexcept {
        return !is_v4_;
    }

    [[nodiscard]]
    sockaddr_in& sockaddr_v4() noexcept {
        return addr_v4_;
    }

    [[nodiscard]]
    socklen_t len() noexcept {
        return len_;
    }

    [[nodiscard]]
    socklen_t* len_addr() noexcept {
        return &len_;
    }

    [[nodiscard]]
    sockaddr_in6& sockaddr_v6() noexcept {
        return addr_v6_;
    }

    [[nodiscard]]
    std::string to_string() const noexcept {
        if (is_ipv4()) {
            char buf[INET_ADDRSTRLEN]{};
            auto& addr = addr_v4_;
            ::inet_ntop(AF_INET, &addr.sin_addr, buf, INET_ADDRSTRLEN);
            return std::format("{}:{}", buf, ::ntohs(addr.sin_port));
        } else {
            char buf[INET6_ADDRSTRLEN]{};
            auto& addr = addr_v6_;
            ::inet_ntop(AF_INET6, &addr.sin6_addr, buf, INET6_ADDRSTRLEN);
            return std::format("{}:{}", buf, ::ntohs(addr.sin6_port));
        }
    }

    [[nodiscard]]
    uint16_t port() const noexcept {
        if (is_ipv4()) {
            return ::ntohs(addr_v4_.sin_port);
        } else {
            return ::ntohs(addr_v6_.sin6_port);
        }
    }

    [[nodiscard]]
    uint16_t nport() const noexcept {
        if (is_ipv4()) {
            return addr_v4_.sin_port;
        } else {
            return addr_v6_.sin6_port;
        }
    }

private:
    bool is_v4_{ false };

    union {
        sockaddr_in addr_v4_;
        sockaddr_in6 addr_v6_;
    };

    socklen_t len_;
};

[[nodiscard]]
SocketAddr make_addr_v4(std::string_view ip_v4, uint16_t port) {
    Ipv4Addr ip = Ipv4Addr::parse(ip_v4);
    return SocketAddr{ ip, port };
}

[[nodiscard]]
SocketAddr make_addr_v6(std::string_view ip_v6, uint16_t port) {
    Ipv6Addr ip = Ipv6Addr::parse(ip_v6);
    return SocketAddr{ ip, port };
}

}  // namespace net::ip
