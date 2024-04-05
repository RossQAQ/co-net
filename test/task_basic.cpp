#include <chrono>

#include "co_net/async_operation/task.hpp"
#include "co_net/context/context.hpp"
#include "co_net/dump.hpp"

using namespace std::chrono_literals;
using namespace tools::debug;
using namespace net::async;

Task<int> write_some() {
    Dump(), "Writting...";
    co_return 10;
}

Task<int> read_some() {
    Dump(), "Reading...";
    co_return 20;
}

Task<void> fake_accept() {
    Dump(), "Accepting a Connection...";
    co_return;
}

Task<void> process() {
    for (;;) {
        co_await read_some();
        Dump(), "Read completed.";
        co_await write_some();
        throw std::runtime_error("wosile");
        Dump(), "Write completed.";
    }
}

Task<void> fake_server() {
    Dump(), std::string("Hello From async main\n\n\n");
    for (size_t i{}; i < 3; ++i) {
        co_await fake_accept();
        std::this_thread::sleep_for(5s);
        co_await process();
    }
}

int main() {
    net::context::Context ctx{};

    return ctx.run(fake_server());
}
