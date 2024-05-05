#pragma once

#include <functional>
#include <tuple>

#include "co_net/async/task.hpp"
#include "co_net/util/noncopyable.hpp"

namespace net::tcp {
class TcpConnection;
};

namespace net::io::msg {

enum class RingMsgType : int32_t {
    RingTask = 0x7a51,
    RingFuncTask = 0x7a51b,
    MultiAccTask = 0x7a51c,
    RingFd = 0xd1fd,
};

struct RingMsgTask : public ::tools::Noncopyable {
    RingMsgTask(async::Task<void> task) : task_(std::move(task)) {}

    ~RingMsgTask() = default;

    auto move_out() noexcept { return std::move(task_); }

private:
    async::Task<void> task_;
};

struct RingMsgConnTaskHelper {
    RingMsgConnTaskHelper(std::function<async::Task<>(net::tcp::TcpConnection)> task, int fd) :
        func_(std::move(task)),
        direct_fd_(fd) {}

    auto move_out() noexcept { return std::move(func_); }

public:
    int direct_fd_{ -1 };

private:
    std::function<async::Task<>(net::tcp::TcpConnection)> func_;
};

struct MultishotAcceptTaskToken {
    MultishotAcceptTaskToken(io::Op op, std::function<async::Task<>(net::tcp::TcpConnection)> task) :
        op_(op),
        func_(std::move(task)) {}

    auto move_out() noexcept { return std::move(func_); }

    io::Op op_;

    std::function<async::Task<>(net::tcp::TcpConnection)> func_;
};

}  // namespace net::io::msg
