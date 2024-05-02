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
    using namespace net::time_literals;
    Dump(), conn.fd();
    for (;;) {
        auto nread = co_await conn.ring_buf_receive();
        if (nread == 0) {
            Dump(), "read = 0";
            break;
        }
        auto msg = conn.move_out_received();
        auto nwrite = co_await conn.write_all(msg);
    }
}

Task<void> server() {
    auto tcp_listener = co_await net::tcp::TcpListener::listen_on(ip::make_addr_v4("localhost", 20589));
    Dump(), tcp_listener.listen_fd();
    for (;;) {
        auto client = co_await tcp_listener.accept();
        net::context::co_spawn(echo(std::move(client)));
        // co_await echo(std::move(client));
        // co_await net::context::parallel_spawn(echo(std::move(client)));
    }
    co_return;
}

int main() {
    return net::context::block_on(server());
}