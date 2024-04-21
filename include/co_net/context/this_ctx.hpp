#pragma once

namespace net::io {

class Uring;

}

namespace net::context {

class SystemLoop;

class TaskLoop;

}  // namespace net::context

namespace this_ctx {

static inline thread_local net::context::TaskLoop* local_task_queue{ nullptr };

static inline thread_local net::io::Uring* local_uring_loop{ nullptr };

}  // namespace this_ctx

namespace sys_ctx {

static inline thread_local net::context::SystemLoop* sys_loop{ nullptr };

static inline thread_local net::context::TaskLoop* sys_task_queue{ nullptr };

static inline thread_local net::io::Uring* sys_uring_loop{ nullptr };

}  // namespace sys_ctx