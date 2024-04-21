#pragma once

#include "co_net/async/task.hpp"
#include "co_net/util/noncopyable.hpp"

namespace net::io::msg {

struct RingMsg : public ::tools::Noncopyable {
    async::Task<void> task_;

    RingMsg(async::Task<void> task) : task_(std::move(task)) {}

    ~RingMsg() = default;

    auto take_out() noexcept { return task_.take(); }
};

}  // namespace net::io::msg
