#include <liburing.h>

#include "co_net/dump.hpp"

using tools::debug::Dump;

int main() {
    io_uring ring;

    io_uring_queue_init(256, &ring, 0);

    io_uring_register_files_sparse(&ring, 512);

    int socket{ -1 };
    {
        auto* sqe = io_uring_get_sqe(&ring);
        io_uring_prep_socket_direct_alloc(sqe, AF_INET, SOCK_STREAM, 0, 0);
        io_uring_submit(&ring);
        io_uring_cqe* cqe;
        io_uring_wait_cqe(&ring, &cqe);
        io_uring_cqe_seen(&ring, cqe);
        socket = cqe->res;
    }

    Dump(), socket;
    sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "localhost", &servaddr.sin_addr.s_addr);
    servaddr.sin_port = htons(port);

    bind(socket, (sockaddr*)&servaddr, sizeof(servaddr));
    listen(socket, 10);

    io_uring_queue_exit(&ring);
}