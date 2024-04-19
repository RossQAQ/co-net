#include <chrono>

#include "co_net/async/task.hpp"
#include "co_net/context/context.hpp"
#include "co_net/dump.hpp"
#include "co_net/net/tcp/listener.hpp"

using namespace tools::debug;
using namespace net;
using namespace net::tcp;
using namespace net::async;

Task<void> server() {
    auto tcp_listener = co_await net::tcp::TcpListener::listen_on(ip::make_addr_v4("127.0.0.1", 20589));
    Dump(), tcp_listener.listen_fd();
    co_return;
}

int main() {
    return net::context::block_on(server());
}
