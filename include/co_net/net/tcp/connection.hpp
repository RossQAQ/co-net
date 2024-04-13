#pragma once

#include <span>

#include "co_net/async/task.hpp"
#include "co_net/net/socket.hpp"
#include "co_net/util/noncopyable.hpp"

namespace net::tcp {

class Connection : public ::tools::Noncopyable {
public:
    explicit Connection(::net::Socket sock) : socket_(std::move(sock)) {}

    ::net::async::Task<ssize_t> read(std::span<char> buf);

    ::net::async::Task<ssize_t> read();

    ::net::async::Task<ssize_t> write(std::span<char> buf, size_t len);

    ::net::async::Task<ssize_t> write(size_t len);

private:
    ::net::Socket socket_;
};

}  // namespace net::tcp
