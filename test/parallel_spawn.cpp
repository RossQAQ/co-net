#include <chrono>
#include <thread>

#include "co_net/async/task.hpp"
#include "co_net/context/context.hpp"
#include "co_net/dump.hpp"
#include "co_net/timer/timer.hpp"

using namespace std::chrono_literals;
using namespace tools::debug;
using namespace net::time_literals;
using namespace net::async;
using namespace net::io::operation;

Task<void> another_process(int) {
    Dump(), "Should be in subthread";
    co_return;
}

Task<void> sub_process() {
    Dump(), "0x5e2d";
    co_return;
}

Task<void> main_process() {
    net::context::parallel_spawn(sub_process());
    net::context::parallel_spawn(another_process(5));
    Dump(), "Main Thread";
    co_return;
}

void funccc() {}

int main() {
    return net::context::block_on(main_process);
}