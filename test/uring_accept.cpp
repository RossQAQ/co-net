#include <arpa/inet.h>
#include <fcntl.h>
#include <liburing.h>
#include <sys/socket.h>

#include "co_net/dump.hpp"

using tools::debug::Dump;

inline int socket_init(int sock, uint16_t port) {
    int opt = 1;
    uint32_t len = sizeof(opt);
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, len);

    sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "localhost", &servaddr.sin_addr.s_addr);
    servaddr.sin_port = htons(port);

    bind(sock, (sockaddr*)&servaddr, sizeof(servaddr));
    listen(sock, 10);

    return sock;
}

int main() {
    io_uring ring;

    io_uring_queue_init(256, &ring, 0);

    io_uring_sqe* sqe_sock = io_uring_get_sqe(&ring);

    io_uring_prep_socket(sqe_sock, AF_INET, SOCK_STREAM, 0, 0);

    io_uring_submit(&ring);

    io_uring_cqe* cqe_sock;

    io_uring_wait_cqe(&ring, &cqe_sock);

    Dump(), cqe_sock->res;

    auto serv = socket_init(cqe_sock->res, 20589);

    io_uring_cqe_seen(&ring, cqe_sock);

    io_uring_sqe* sqe = io_uring_get_sqe(&ring);

    sockaddr_in addr;
    socklen_t len;
    io_uring_prep_multishot_accept(sqe, serv, nullptr, nullptr, 0);

    io_uring_submit(&ring);

    Dump(), "Submitted";

    io_uring_cqe* cqe;

    Dump(), "Waiting for CQE";

    io_uring_wait_cqe(&ring, &cqe);

    Dump(), cqe->res;

    Dump(), getpeername(cqe->res, (sockaddr*)&addr, &len);

    io_uring_cqe_seen(&ring, cqe);

    char buf[INET_ADDRSTRLEN]{};
    ::inet_ntop(AF_INET, &addr.sin_addr, buf, INET_ADDRSTRLEN);

    Dump(), buf;

    close(serv);

    io_uring_queue_exit(&ring);
}