#include <chrono>

#include "co_net/async/task.hpp"
#include "co_net/context/context.hpp"
#include "co_net/dump.hpp"
#include "co_net/io/prep/prep_socket.hpp"
#include "co_net/timer/timer.hpp"

using namespace std::chrono_literals;
using namespace tools::debug;
using namespace net::time_literals;
using namespace net::async;
using namespace net::io::operation;

using net::context::co_spawn;

Task<void> direct_sock() {
    auto sockfd = co_await net::io::operation::prep_normal_socket(AF_INET, SOCK_STREAM, 0, 0);
    Dump(), sockfd;
}

int main() {
    return net::context::block_on(direct_sock());
}
