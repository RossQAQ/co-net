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

Task<> world() {
    Dump(), "World";
    co_return;
}

Task<> hello() {
    Dump(), "Hello";
    co_await world();
    co_return;
}

Task<> spawn_coroutines(int x) {
    Dump(), x;
    net::context::co_spawn(&hello);
    net::context::co_spawn(&hello);
    co_return;
}

int main() {
    return net::context::block_on(&spawn_coroutines, 42);
}