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

class Entity {
public:
    Entity() {}
    Entity(const Entity&) {}
    Entity(Entity&&) {}
    ~Entity() { Dump(); }
};

Task<> world(Entity e) {
    Dump(), "World";
    co_return;
}

Task<> hello(Entity e) {
    Dump(), "Hello";
    co_await world(std::move(e));
    co_return;
}

Task<> spawn_coroutines(int x) {
    Dump(), x;
    Entity e;
    net::context::co_spawn(&hello, e);
    net::context::co_spawn(&hello, e);
    co_return;
}

int main() {
    return net::context::block_on(&spawn_coroutines, 42);
}