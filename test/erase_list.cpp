#include <algorithm>
#include <list>

#include "co_net/dump.hpp"

using tools::debug::Dump;

struct Entity {
    bool valid{ true };
    ~Entity() { Dump(); }
};

int main() {
    std::list<Entity> lst;

    lst.push_back(Entity{});
    lst.push_back(Entity{});
    lst.push_back(Entity{ false });

    Dump(), lst.size();

    lst.erase(std::remove_if(lst.begin(), lst.end(), [](Entity& e) { return e.valid; }), lst.end());

    Dump(), lst.size();
}