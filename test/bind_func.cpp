#include <functional>

#include "co_net/dump.hpp"

using tools::debug::Dump;

void func(int x, double y, float z) {
    Dump(), x, y, z;
}

int main() {
    std::function<void(float)> f = std::bind(&func, 1, 2, std::placeholders::_1);

    auto f2 = std::move(f);

    // f(10);

    f2(999);
}