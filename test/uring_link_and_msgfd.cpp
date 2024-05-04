#include <arpa/inet.h>
#include <liburing.h>
#include <sys/socket.h>

#include <chrono>
#include <thread>

#include "co_net/dump.hpp"
#include "co_net/timer/timer.hpp"

using namespace std::chrono_literals;
using namespace net::time_literals;
using tools::debug::Dump;

// {
//     auto* sqe_01 = io_uring_get_sqe(&ring_01);
//     auto* sqe_02 = io_uring_get_sqe(&ring_01);
//     auto* sqe_03 = io_uring_get_sqe(&ring_01);

//     auto ts_5s = 5_s;
//     auto ts_3s = 3_s;
//     auto ts_1s = 1_s;
//     io_uring_prep_timeout(sqe_01, &ts_5s, 0, IORING_TIMEOUT_REALTIME | IORING_TIMEOUT_ETIME_SUCCESS);
//     io_uring_prep_timeout(sqe_02, &ts_1s, 0, IORING_TIMEOUT_REALTIME);
//     io_uring_prep_timeout(sqe_03, &ts_3s, 0, IORING_TIMEOUT_REALTIME);

//     io_uring_sqe_set_data64(sqe_01, 1);
//     io_uring_sqe_set_data64(sqe_02, 2);
//     io_uring_sqe_set_data64(sqe_03, 3);

//     sqe_01->flags |= IOSQE_IO_LINK;

//     io_uring_submit(&ring_01);

//     for (size_t i{}; i < 3; ++i) {
//         io_uring_cqe* cqe;
//         io_uring_wait_cqe(&ring_01, &cqe);
//         Dump(), strerror(-cqe->res), cqe->user_data;
//         io_uring_cqe_seen(&ring_01, cqe);
//     }
// }

inline int socket_init(uint16_t port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    uint32_t len = sizeof(opt);
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, len);

    sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    bind(sock, (sockaddr*)&servaddr, sizeof(servaddr));
    listen(sock, 10);

    return sock;
}

int main() {
    int ring_02_fd{};

    int ring_03_fd{};

    auto loop = [&ring_02_fd] {
        io_uring ring_02;
        io_uring_queue_init(256, &ring_02, 0);
        io_uring_register_ring_fd(&ring_02);

        ring_02_fd = ring_02.ring_fd;

        io_uring_register_files_sparse(&ring_02, 1024);

        int last_sock{};

        for (;;) {
            io_uring_cqe* cqe;

            io_uring_wait_cqe(&ring_02, &cqe);

            if (cqe->user_data == 5005) {
                Dump(), "Lambda: ring_02 Direct Socket", cqe->res;
                io_uring_sqe* sqe = io_uring_get_sqe(&ring_02);
                io_uring_prep_write(sqe, cqe->res, "Hello World\n", 13, 0);
                io_uring_sqe_set_data64(sqe, 7777);
                sqe->flags |= IOSQE_FIXED_FILE;
                last_sock = cqe->res;
            }

            if (cqe->user_data == 7777) {
                std::this_thread::sleep_for(3s);
                io_uring_sqe* sqe = io_uring_get_sqe(&ring_02);
                io_uring_prep_write(sqe, last_sock, "Hello World\n", 13, 0);
                io_uring_sqe_set_data64(sqe, 1919);
                sqe->flags |= IOSQE_FIXED_FILE;
                // Dump(), "Lambda: ring_02 Writed";
            }

            if (cqe->user_data == 1919) {
                Dump(), "S b?";
                io_uring_sqe* sqe2 = io_uring_get_sqe(&ring_02);
                io_uring_prep_close_direct(sqe2, last_sock);
                io_uring_sqe_set_data64(sqe2, 4444);
            }

            if (cqe->user_data == 4444) {
                Dump(), "Close Socket", last_sock;
            }

            io_uring_cqe_seen(&ring_02, cqe);

            io_uring_submit(&ring_02);
        }

        io_uring_queue_exit(&ring_02);
    };

    // auto loop = [&ring_03_fd] {
    //     io_uring ring_03;
    //     io_uring_queue_init(256, &ring_03, 0);
    //     io_uring_register_ring_fd(&ring_03);

    //     ring_03_fd = ring_03.ring_fd;

    //     io_uring_register_files_sparse(&ring_03, 1024);

    //     int last_sock{};

    //     for (;;) {
    //         io_uring_cqe* cqe;

    //         io_uring_wait_cqe(&ring_03, &cqe);

    //         std::string hello{ "Hello World" };

    //         if (cqe->user_data == 5005) {
    //             Dump(), "Lambda: ring_03 Direct Socket", cqe->res;
    //             io_uring_sqe* sqe = io_uring_get_sqe(&ring_03);
    //             io_uring_prep_write(sqe, cqe->res, hello.c_str(), hello.size(), 0);
    //             io_uring_sqe_set_data64(sqe, 7777);
    //             last_sock = cqe->res;
    //         }

    //         if (cqe->user_data == 7777) {
    //             Dump(), "Lambda: ring_03, wrtied";
    //             io_uring_sqe* sqe = io_uring_get_sqe(&ring_03);
    //             io_uring_prep_close_direct(sqe, last_sock);
    //             io_uring_sqe_set_data64(sqe, 1919);
    //         }

    //         if (cqe->user_data == 1919) {
    //             Dump(), "Close Socket";
    //         }

    //         io_uring_cqe_seen(&ring_03, cqe);

    //         io_uring_submit(&ring_03);
    //     }

    //     io_uring_queue_exit(&ring_03);
    // };

    io_uring ring_01;

    io_uring_queue_init(256, &ring_01, 0);

    io_uring_register_ring_fd(&ring_01);

    io_uring_register_files_sparse(&ring_01, 1024);

    auto sock = socket_init(20589);

    std::thread th{ loop };

    th.detach();

    {
        auto* sqe_01 = io_uring_get_sqe(&ring_01);
        auto* sqe_02 = io_uring_get_sqe(&ring_01);
        auto* sqe_03 = io_uring_get_sqe(&ring_01);

        auto ts_5s = 5_s;
        auto ts_3s = 3_s;
        auto ts_1s = 1_s;
        io_uring_prep_timeout(sqe_01, &ts_5s, 0, IORING_TIMEOUT_REALTIME | IORING_TIMEOUT_ETIME_SUCCESS);
        io_uring_prep_timeout(sqe_02, &ts_1s, 0, IORING_TIMEOUT_REALTIME);
        io_uring_prep_timeout(sqe_03, &ts_3s, 0, IORING_TIMEOUT_REALTIME);

        io_uring_sqe_set_data64(sqe_01, 1);
        io_uring_sqe_set_data64(sqe_02, 2);
        io_uring_sqe_set_data64(sqe_03, 3);

        sqe_01->flags |= IOSQE_CQE_SKIP_SUCCESS;

        io_uring_submit(&ring_01);

        for (size_t i{}; i < 3; ++i) {
            io_uring_cqe* cqe;
            io_uring_wait_cqe(&ring_01, &cqe);
            Dump(), strerror(-cqe->res), cqe->user_data;
            io_uring_cqe_seen(&ring_01, cqe);
        }
    }

    {
        auto* sqe_acc = io_uring_get_sqe(&ring_01);

        io_uring_prep_multishot_accept_direct(sqe_acc, sock, nullptr, nullptr, 0);

        io_uring_sqe_set_data64(sqe_acc, 1234);

        int last_sock{};

        for (size_t i{}; true; i++) {
            io_uring_submit(&ring_01);

            io_uring_cqe* cqe;
            auto ret = io_uring_wait_cqe(&ring_01, &cqe);
            if (cqe->user_data == 8848) {
                Dump(), cqe->res;
            }

            if (cqe->user_data == 4444) {
                Dump(), "Should closed";
                std::this_thread::sleep_for(8s);

                io_uring_sqe* write_sqe = io_uring_get_sqe(&ring_01);
                io_uring_prep_write(write_sqe, last_sock, "不应该出现\n", 17, 0);
                io_uring_sqe_set_data64(write_sqe, 8848);
                write_sqe->flags |= IOSQE_FIXED_FILE;
            }

            if (cqe->user_data == 9876) {
                std::this_thread::sleep_for(8s);
                Dump(), "Should close: ", last_sock;
                io_uring_sqe* close_sqe = io_uring_get_sqe(&ring_01);
                io_uring_prep_close_direct(close_sqe, last_sock);
                io_uring_sqe_set_data64(close_sqe, 4444);
            }

            if (cqe->user_data == 7890) {
                if (cqe->res >= 0)
                    Dump(), "Main: Msg Fd Send";
                else
                    Dump(), strerror(cqe->res);

                io_uring_sqe* write_sqe = io_uring_get_sqe(&ring_01);
                io_uring_prep_write(write_sqe, last_sock, "Ri\n", 4, 0);
                io_uring_sqe_set_data64(write_sqe, 9876);
                write_sqe->flags |= IOSQE_FIXED_FILE;
            }

            if (cqe->user_data == 1234 && (cqe->flags & IORING_CQE_F_MORE)) {
                Dump(), "Main Accept: ", cqe->res;
                Dump(), "Main: send a ring msg, fd:", cqe->res;
                last_sock = cqe->res;

                io_uring_sqe* msg_fd_sqe = io_uring_get_sqe(&ring_01);
                io_uring_prep_msg_ring_fd_alloc(msg_fd_sqe, ring_02_fd, last_sock, 5005, 0);
                io_uring_sqe_set_data64(msg_fd_sqe, 7890);
            }

            io_uring_cqe_seen(&ring_01, cqe);
        }
    }
    io_uring_queue_exit(&ring_01);
}