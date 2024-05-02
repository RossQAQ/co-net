#include <chrono>
#include <vector>

#include "co_net/async/task.hpp"
#include "co_net/context/context.hpp"
#include "co_net/dump.hpp"
#include "co_net/io/prep/prep_accept.hpp"
#include "co_net/net/tcp/listener.hpp"

using namespace tools::debug;
using namespace net;
using namespace net::tcp;
using namespace net::async;

Task<void> show_address(TcpConnection conn) {
    for (;;) {
        auto nread = co_await conn.receive();
        Dump(), nread, "have read.";
        if (nread == 0) {
            co_return;
        }
    }
}

Task<void> server() {
    auto tcp_listener = co_await net::tcp::TcpListener::listen_on(ip::make_addr_v4("localhost", 20589));
    for (;;) {
        auto client = co_await tcp_listener.accept();
        Dump(), "Client: ", client.fd();
        net::context::co_spawn(show_address(std::move(client)));
    }
    co_return;
}

int main() {
    return net::context::block_on(server());
}