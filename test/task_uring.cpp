#include <chrono>

#include "co_net/async/task.hpp"
#include "co_net/context/context.hpp"
#include "co_net/dump.hpp"
#include "co_net/io/prep/prep_timeout.hpp"
#include "co_net/timer/timer.hpp"

using namespace std::chrono_literals;
using namespace tools::debug;
using namespace net::time::literals;
using namespace net::async;
using namespace net::io::operation;

Task<void> timeout() {
    Dump(), "Prepare into uring";
    co_await uring_prep_timer_once(3s);
}

Task<void> wait_for_conn() {
    Dump(), "Before timeout";
    co_await timeout();
    Dump(), "After 3s";
}

Task<void> fake_server() {
    Dump(), "Before wait conn";
    co_await timeout();
    Dump(), "After wait conn";
}

int main() {
    return net::context::block_on(fake_server());
}
