#pragma once

#include <coroutine>

namespace net::io {

struct CallerCoro {
    std::coroutine_handle<> handle_{ nullptr };
};

}  // namespace net::io