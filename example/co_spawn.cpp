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

Task<void> say_hello() {
    Dump(), "Hey";
    co_return;
}

Task<void> server() {
    net::context::co_spawn(say_hello());
    net::context::co_spawn(say_hello());
    net::context::co_spawn(say_hello());
    net::context::co_spawn(say_hello());
    net::context::co_spawn(say_hello());
    Dump(), this_ctx::local_task_queue->size();
    co_return;
}

int main() {
    return net::context::block_on(server());
}