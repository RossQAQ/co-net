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

using net::context::co_spawn;

Task<void> finish_A() {
    Dump(), "Finish A";
    co_return;
}

Task<void> finish_B() {
    Dump(), "Finish B";
    co_return;
}

Task<void> process_A() {
    Dump(), "I'm processing A";
    co_await finish_A();
    co_return;
}

Task<void> process_B() {
    Dump(), "I'm processing B";
    co_await finish_B();
    co_return;
}

Task<void> loop_processing() {
    for (;;) {
        co_spawn(process_A());
        co_spawn(process_B());
        Dump(), "Sleeping";
        co_await async_sleep_for(3s);
        Dump(), "Wake";
    }
}

int main() {
    return net::context::block_on(loop_processing());
}
