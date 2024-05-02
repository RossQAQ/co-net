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
    ~Entity() {}
};

class Conn {
public:
    Conn(Entity e) : e_(std::move(e)) {}

    Conn(Conn&&) = default;

    ~Conn() { Dump(); }

private:
    Entity e_;
};

Task<void> echo(Conn conn) {
    Dump(), "this is echo";
    co_return;
}

Task<void> server() {
    Entity e;
    Dump();
    co_await echo(std::move(e));
    co_return;
}

int main() {
    int ret = net::context::block_on(server());
    Dump();
    return ret;
}