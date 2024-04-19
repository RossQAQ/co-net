#include <chrono>

#include "co_net/async/task.hpp"
#include "co_net/context/context.hpp"
#include "co_net/dump.hpp"

using namespace std::chrono_literals;
using namespace tools::debug;
using namespace net::async;

Task<int> hello() {
    co_return 42;
}

Task<void> async_main() {
    co_await hello();
    for (;;) {}
}

int main() {
    return net::context::block_on(async_main());
}