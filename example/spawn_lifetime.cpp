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

class Conn {
public:
    Conn(Entity e) : e_(std::move(e)) {}

    Conn(Conn&&) = default;

    ~Conn() { Dump(), "Sha bi~"; }

private:
    Entity e_;
};

Task<void> echo(Conn conn) {
    co_return;
}

Task<void> server() {
    Entity e;
    net::context::co_spawn(echo(std::move(e)));
    co_return;
}

int main() {
    Dump(), sizeof(Task<>);
    return net::context::block_on(server());
}