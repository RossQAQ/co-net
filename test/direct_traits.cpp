#include <vector>

#include "co_net/async/task.hpp"
#include "co_net/dump.hpp"
#include "co_net/net/socket.hpp"
#include "co_net/net/tcp/connection.hpp"
#include "co_net/traits.hpp"

using namespace tools::debug;
using net::async::Task;
using net::traits::has_direct_fd_v;

int main() {
    Dump(), has_direct_fd_v<int, double>;
    Dump(), has_direct_fd_v<float>;
    Dump(), has_direct_fd_v<net::tcp::TcpConnection&, int>;
    Dump(), has_direct_fd_v<net::tcp::TcpConnection&&, int>;
    Dump(), has_direct_fd_v<int, net::tcp::TcpConnection>;
    Dump(), has_direct_fd_v<net::tcp::TcpConnection, net::DirectSocket>;
    Dump(), has_direct_fd_v<float, net::DirectSocket>;
    return 0;
}