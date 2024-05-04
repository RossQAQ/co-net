#include <liburing.h>

#include "co_net/dump.hpp"

using tools::debug::Dump;

struct RingMsg {};

template <typename VoidTask, typename... Args>
    requires(std::is_invocable_r_v<void, VoidTask, Args...> && std::is_rvalue_reference_v<VoidTask &&>)
struct RingMsgTask {
    RingMsgTask(VoidTask&& callable, Args&&... args) :
        task_callable_(std::forward<VoidTask>(callable)),
        args_(std::make_tuple(std::forward<Args>(args)...)) {
        Dump(), "args";
    }

    VoidTask&& task_callable_;
    std::tuple<Args...> args_;
};

void snd_func(int x, double y) {
    Dump(), x, y;
}

template <typename VoidTask, typename... Args>
void handle_send_function() {}

int main() {
    io_uring ring_01;
    io_uring ring_02;

    io_uring_queue_init(256, &ring_01, 0);
    io_uring_queue_init(256, &ring_02, 0);

    io_uring_register_ring_fd(&ring_01);
    io_uring_register_ring_fd(&ring_02);

    io_uring_register_files_sparse(&ring_01, 1024);

    {
        auto* sqe = io_uring_get_sqe(&ring_01);
        io_uring_prep_msg_ring_cqe_flags(sqe, ring_02.ring_fd, 999, 999, 0, 0);
        io_uring_sqe_set_data64(sqe, 1234);

        auto* ring_task = new RingMsgTask(&snd_func, 1, 2.0);

        io_uring_submit(&ring_01);

        io_uring_cqe* cqe;
        io_uring_wait_cqe(&ring_01, &cqe);
        Dump(), "This is ring: ", ring_01.ring_fd;
        Dump(), cqe->res, cqe->user_data;
        io_uring_cqe_seen(&ring_01, cqe);
    }

    {
        auto* sqe_acc = io_uring_get_sqe(&ring_01);
        io_uring_prep_accept_direct(sqe_acc, );
    }

    {
        io_uring_cqe* cqe;
        io_uring_wait_cqe(&ring_02, &cqe);
        Dump(), "This is ring: ", ring_02.ring_fd;
        Dump(), cqe->res, cqe->user_data;
        io_uring_cqe_seen(&ring_02, cqe);
    }

    io_uring_queue_exit(&ring_01);
    io_uring_queue_exit(&ring_02);
}