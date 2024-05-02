#include <vector>

#include "co_net/async/task.hpp"
#include "co_net/context/context.hpp"
#include "co_net/dump.hpp"
#include "co_net/net/tcp/listener.hpp"

using namespace tools::debug;
using namespace net;
using namespace net::tcp;
using namespace net::async;

Task<void> echo(TcpConnection conn) {
    Dump(), "In subthread";
    std::vector<char> buffer(1024);
    for (;;) {
        auto nread = co_await conn.read_some(buffer);
        Dump(), nread, "Bytes receive.";
        if (nread == 0) {
            break;
        }
        auto nwrite = co_await conn.write_some(buffer, nread);
        Dump(), nwrite, "Bytes send.";
    }
}

Task<void> server() {
    auto tcp_listener = co_await net::tcp::TcpListener::listen_on(ip::make_addr_v4("localhost", 20589));
    Dump(), "Server ring fd: ", this_ctx::local_uring_loop->fd(), sys_ctx::sys_uring_loop->fd();
    for (;;) {
        auto client = co_await tcp_listener.accept();
        Dump(), "Client: ", client.fd();
        net::context::co_spawn(echo(std::move(client)));
        // co_await net::context::parallel_spawn(echo(std::move(client)));
    }
    co_return;
}

int main() {
    return net::context::block_on(server());
}