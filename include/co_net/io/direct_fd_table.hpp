#pragma once

// !todo

#include <memory>

#include "co_net/config.hpp"

namespace net::io {

class DirectFds {
public:
private:
    std::unique_ptr<int[]> fds_;
};

}  // namespace net::io
