#include <chrono>

#include "co_net/async/task.hpp"
#include "co_net/context/context.hpp"
#include "co_net/dump.hpp"
#include "co_net/timer/timer.hpp"

using namespace std::chrono_literals;
using namespace tools::debug;
using namespace net::time_literals;
using namespace net::async;
using namespace net::io::operation;

Task<void> concurrent_task() {
    Dump(), "Concurrent";
}

Task<void> wait_for_conn() {
    Dump(), "Before timeout";
    net::context::co_spawn(async_sleep_for(1s));
    net::context::co_spawn(async_sleep_for(3s));
    co_return;
}

Task<void> fake_server() {
    Dump(), "Before wait conn";
    co_await wait_for_conn();
    Dump(), "After wait conn";
}

int main() {
    return net::context::block_on(fake_server());
}
