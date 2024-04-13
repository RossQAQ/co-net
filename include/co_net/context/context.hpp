#pragma once

#include <thread>

#include "co_net/async/task.hpp"
#include "co_net/config.hpp"
#include "co_net/context/task_loop.hpp"
#include "co_net/context/thread_pool.hpp"
#include "co_net/dump.hpp"
#include "co_net/io/uring.hpp"

namespace net::context {

class EventLoop : tools::Noncopyable {
public:
    EventLoop(net::io::UringType uring_type) : task_loop_(), uring_(uring_type, &task_loop_) {}

    void submit_task(std::coroutine_handle<> task) { task_loop_.enqueue(task); }

    [[nodicard]]
    ::net::io::Uring& get_uring_loop() noexcept {
        return uring_;
    }

    [[nodicard]]
    ::net::context::TaskLoop& get_task_loop() noexcept {
        return task_loop_;
    }

    [[nodicard]]
    const ::net::io::Uring& get_uring_loop() const noexcept {
        return uring_;
    }

    [[nodicard]]
    const ::net::context::TaskLoop& get_task_loop() const noexcept {
        return task_loop_;
    }

    void run() {
        for (;;) {
            task_loop_.run();
            uring_.submit_all();
            uring_.wait_one_cqe();
        }
    }

private:
    ::net::context::TaskLoop task_loop_;

    ::net::io::Uring uring_;
};

inline thread_local EventLoop loop{ net::io::UringType::Major };

inline void co_spawn(async::Task<void>&& task) {
    auto handle = task.take();
    loop.submit_task(handle);
}

[[nodiscard]]
inline int block_on(async::Task<void>&& task) {
    loop.submit_task(task.handle());
    loop.run();
    return 0;
}

}  // namespace net::context
