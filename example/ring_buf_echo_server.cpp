#include <vector>

#include "co_net/async/task.hpp"
#include "co_net/context/context.hpp"
#include "co_net/dump.hpp"
#include "co_net/net/tcp/listener.hpp"
#include "co_net/timer/timer.hpp"

using namespace tools::debug;
using namespace net;
using namespace net::tcp;
using namespace net::async;

Task<void> echo(TcpConnection conn) {
    Dump(), conn.fd();
    for (;;) {
        auto nread = co_await conn.ring_buf_receive();
        if (!nread) {
            co_return;
        }
        auto msg = conn.move_out_received();
        auto nwrite = co_await conn.write_all(msg);
    }
}

Task<void> server() {
    auto tcp_listener = co_await net::tcp::TcpListener::listen_on(ip::make_addr_v4("localhost", 20589));
    co_await tcp_listener.multishot_accept_then(&echo);
    // for (;;) {
    //     auto client = co_await tcp_listener.accept();
    //     net::context::co_spawn(&echo, std::move(client));
    //     net::context::parallel_spawn(&echo, std::move(client));
    // }
}

int main() {
    return net::context::block_on(&server);
}