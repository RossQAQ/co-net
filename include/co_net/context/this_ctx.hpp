#pragma once

#include "co_net/context/task_loop.hpp"
#include "co_net/io/uring.hpp"

namespace this_ctx {

static inline thread_local net::context::TaskLoop* local_task_queue{ nullptr };

static inline thread_local net::io::Uring* local_uring_loop{ nullptr };

}  // namespace this_ctx

namespace system {

static inline thread_local net::context::TaskLoop* sys_task_queue{ nullptr };

static inline thread_local net::io::Uring* sys_uring_loop{ nullptr };

}  // namespace system