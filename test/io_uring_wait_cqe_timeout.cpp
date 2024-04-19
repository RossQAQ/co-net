#include <liburing.h>

#include <iostream>

#include "co_net/timer/timer.hpp"

using namespace net::time_literals;

inline constexpr __kernel_timespec MINOR_URING_WAIT_CQE_TIMEOUT = 3s;

int main() {
    io_uring ring;

    io_uring_queue_init(256, &ring, 0);

    io_uring_cqe* cqe;

    __kernel_timespec time_spec = 3s;

    auto cnt = io_uring_wait_cqe_timeout(&ring, &cqe, &time_spec);

    std::cout << cnt;

    return 0;
}