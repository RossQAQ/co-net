#pragma once

#include <stdint.h>

#include <coroutine>

namespace net::io {

struct CompletionToken {
    std::coroutine_handle<> handle_{ nullptr };
    int32_t cqe_res_{ -ENOSYS };
    int32_t cqe_flags_;
};

}  // namespace net::io