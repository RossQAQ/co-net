/**
 * @file context.hpp
 * @author Roseveknif (rossqaq@outlook.com)
 * @brief io_context
 * @version 0.1
 * @date 2024-04-03
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <thread>

#include "co_net/async_operation/task.hpp"
#include "co_net/context/scheduler.hpp"
#include "co_net/context/thread_pool.hpp"

namespace net::context {

class Context : public tools::Noncopyable {
public:
    Context(thread_t n = 0) : pool_(n), scheduler_() {}

    ~Context() = default;

    int run(async::Task<void>&& start_task) {
        scheduler_.submit(start_task.handle());

        scheduler_.loop();

        return 0;
    }

private:
    ThreadPool pool_;
    Scheduler scheduler_;
};

}  // namespace net::context
