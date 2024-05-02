#include <vector>

#include "co_net/async/task.hpp"
#include "co_net/context/task_loop.hpp"
#include "co_net/dump.hpp"

using net::async::Task;

static net::context::TaskLoop loop;

class Entity {
public:
    Entity() { Dump(); }
    Entity(const Entity&) { Dump(); }
    Entity(Entity&&) { Dump(); }
    ~Entity() { Dump(); }
};

void co_spawn(Task<void>&& task) {
    auto handle = task.take();
    loop.enqueue(handle);
}

Task<void> client_process(Entity&& entity) {
    Dump(), "Processing";
    co_return;
}

Task<void> server(Entity e) {
    // Entity client;
    // co_spawn(client_process(std::move(client)));
    Dump(), "in server";
    co_return;
}

int main() {
    // auto task = server();
    // auto handle = task.take();
    Entity e;
    Dump(), "Before co_spawn";
    co_spawn(server(std::move(e)));
    loop.run_test();
    Dump(), "Before return";
    return 0;
}