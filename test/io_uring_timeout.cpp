#include <liburing.h>
#include <string.h>
#include <sys/time.h>

#include "co_net/dump.hpp"

using tools::debug::Dump;

int main() {
    __kernel_timespec tm;
    tm.tv_sec = 1;
    tm.tv_nsec = 0;

    io_uring ring;
    io_uring_queue_init(256, &ring, 0);

    auto* sqe = io_uring_get_sqe(&ring);

    Dump(), "Timeout only once, after 1s";

    io_uring_prep_timeout(sqe, &tm, 1, IORING_TIMEOUT_REALTIME | IORING_TIMEOUT_ETIME_SUCCESS);

    io_uring_sqe_set_data64(sqe, 1);

    io_uring_submit(&ring);

    io_uring_cqe* cqe;

    io_uring_wait_cqe(&ring, &cqe);

    Dump(), "Timeout";

    Dump(), strerror(-cqe->res), cqe->user_data;
}