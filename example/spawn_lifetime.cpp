#include <vector>

#include "co_net/async/task.hpp"
#include "co_net/context/context.hpp"
#include "co_net/dump.hpp"
#include "co_net/net/tcp/listener.hpp"

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

Task<void> echo(Entity conn) {
    Dump(), "this is echo";
    co_return;
}

Task<void> server() {
    Entity e1;
    Entity e2;
    Entity e3;
    Dump();
    net::context::parallel_spawn(&echo, std::move(e1));
    net::context::parallel_spawn(&echo, std::move(e2));
    net::context::parallel_spawn(&echo, std::move(e3));
    for (;;) {}
    co_return;
}

int main() {
    int ret = net::context::block_on(&server);
    return ret;
}