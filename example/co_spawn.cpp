#include <vector>

#include "co_net/async/task.hpp"
#include "co_net/context/context.hpp"
#include "co_net/dump.hpp"
#include "co_net/io/prep/prep_timeout.hpp"
#include "co_net/net/tcp/listener.hpp"
#include "co_net/timer/timer.hpp"

using namespace tools::debug;
using namespace net;
using namespace net::tcp;
using namespace net::async;
using namespace net::time_literals;
using net::io::operation::async_sleep_for;

Task<> world() {
    Dump(), "World";
    co_return;
}

Task<> hello() {
    Dump(), "";
    co_await async_sleep_for(20_ms);
    Dump(), "hello";
    co_return;
}

Task<> spawn_coroutines() {
    for (size_t i{}; i < 1; ++i) {
        Dump(), "";
        co_await async_sleep_for(10_ms);
        net::context::co_spawn(&hello);
        // co_await async_sleep_for(50_ms);
    }
    Dump(), "wake";
    co_return;
}

int main() {
    return net::context::block_on(&spawn_coroutines);
}