#include "co_net/co_net.hpp"

using namespace net;
using namespace net::tcp;
using namespace net::async;

Task<void> echo(TcpConnection conn) {
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
}

int main() {
    return net::context::block_on(&server);
}