#include <liburing.h>

#include <span>
#include <string>
#include <vector>

#include "co_net/dump.hpp"

using tools::debug::Dump;

static int NBUFS = 1;

static int BUF_SIZE = 1024;

static int count = 1;

int main() {
    io_uring ring;

    io_uring_queue_init(256, &ring, 0);

    std::vector<char> buf(NBUFS * BUF_SIZE * count);

    int res{};

    auto* br_ = io_uring_setup_buf_ring(&ring, NBUFS, 0, 0, &res);

    if (res < 0) {
        Dump(), strerror(-res);
        throw std::runtime_error("Buffer Error.");
    }

    int mask = io_uring_buf_ring_mask(NBUFS);

    char* ptr = buf.data();

    for (size_t i{}; i < NBUFS; ++i) {
        io_uring_buf_ring_add(br_, ptr, BUF_SIZE, i, mask, i);
        ptr += BUF_SIZE;
    }

    io_uring_buf_ring_advance(br_, NBUFS);

    Dump(), buf.size();

    io_uring_free_buf_ring(&ring, br_, NBUFS, 0);

    count *= 2;

    buf.resize(NBUFS * BUF_SIZE * count);

    {
        int res{};

        auto* br_ = io_uring_setup_buf_ring(&ring, NBUFS * count, 0, 0, &res);

        if (res < 0) {
            Dump(), strerror(-res);
            throw std::runtime_error("Buffer Error.");
        }

        Dump(), buf.size();

        int mask = io_uring_buf_ring_mask(NBUFS);

        char* ptr = buf.data();

        for (size_t i{}; i < NBUFS; ++i) {
            io_uring_buf_ring_add(br_, ptr, BUF_SIZE, i, mask, i);
            ptr += BUF_SIZE;
        }

        io_uring_buf_ring_advance(br_, NBUFS);

        io_uring_free_buf_ring(&ring, br_, NBUFS, 0);
    }

    io_uring_queue_exit(&ring);

    return 0;

    // --------------------------------------------------------------

    int fd = open("/home/ecs-user/co-net/sample_text/one_sentence", O_RDONLY);

    int offset = 0;

    int nbytes{};

    std::vector<int> chosed_idx;
    chosed_idx.reserve(16);

    for (;;) {
        io_uring_sqe* sqe = io_uring_get_sqe(&ring);

        io_uring_prep_read(sqe, fd, nullptr, 0, offset);

        sqe->flags |= IOSQE_BUFFER_SELECT;

        io_uring_submit(&ring);

        io_uring_cqe* cqe{};

        io_uring_wait_cqe(&ring, &cqe);

        io_uring_cqe_seen(&ring, cqe);

        chosed_idx.push_back(cqe->flags >> IORING_CQE_BUFFER_SHIFT);

        if (cqe->res == 0) {
            break;
        }

        nbytes += cqe->res;

        offset += BUF_SIZE;
    }

    Dump(), "First Read Completed.", nbytes, "Bytes";

    Dump(), "Cosume", offset / BUF_SIZE;

    std::vector<char> conn_buf{};

    conn_buf.reserve(chosed_idx.size());

    for (auto idx : chosed_idx) {
        std::copy(buf.begin() + idx * BUF_SIZE, buf.begin() + idx * BUF_SIZE + BUF_SIZE, std::back_inserter(conn_buf));
    }

    Dump(), conn_buf.data();

    // --------------------------------------------------------------------------

    {
        int nr_packets{};

        for (auto idx : chosed_idx) {
            char* this_buf = buf.data() + idx * BUF_SIZE;

            io_uring_buf_ring_add(br_, static_cast<void*>(this_buf), BUF_SIZE, idx, mask, nr_packets);

            nr_packets++;
        }

        io_uring_buf_ring_advance(br_, nr_packets);
    }

    offset = 0;

    for (;;) {
        io_uring_sqe* sqe = io_uring_get_sqe(&ring);

        io_uring_prep_read(sqe, fd, nullptr, 0, offset);

        sqe->flags |= IOSQE_BUFFER_SELECT;

        io_uring_submit(&ring);

        io_uring_cqe* cqe{};

        io_uring_wait_cqe(&ring, &cqe);

        io_uring_cqe_seen(&ring, cqe);

        // Dump(), cqe->res, cqe->flags >> IORING_CQE_BUFFER_SHIFT, cqe->flags & IORING_CQE_F_BUFFER;

        Dump(), "Choose Buffer: ", cqe->flags >> IORING_CQE_BUFFER_SHIFT;

        if (cqe->res == 0) {
            break;
        }

        if (cqe->res == -ENOBUFS) {
            Dump(), "No bufs";
            break;
        }

        nbytes += cqe->res;

        offset += BUF_SIZE;
    }

    io_uring_free_buf_ring(&ring, br_, NBUFS, 0);

    io_uring_queue_exit(&ring);
}