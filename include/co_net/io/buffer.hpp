#pragma once

#include <liburing.h>

#include <atomic>
#include <vector>

#include "co_net/config.hpp"
#include "co_net/util/noncopyable.hpp"

namespace net::io {

class RingBuffer : public tools::Noncopyable {
public:
    RingBuffer(io_uring* ring) : peer_ring_(ring) { extend_buffer(); }

    ~RingBuffer() { release(); }

    int mask() const noexcept { return br_mask_; }

    int group_id() const noexcept { return peer_ring_->ring_fd; }

    void copy_data(std::vector<char>& output, int bid, int bytes) {
        int offset = bid * config::BUFFER_RING_SIZE;
        std::copy(buffer_.begin() + offset, buffer_.begin() + offset + bytes, std::back_inserter(output));
    }

    void repay(int bid) {
        char* this_buf = buffer_.data() + bid * config::BUFFER_RING_SIZE;

        io_uring_buf_ring_add(br_, static_cast<void*>(this_buf), config::BUFFER_RING_SIZE, bid, br_mask_, 0);

        io_uring_buf_ring_advance(br_, 1);
    }

    void extend_buffer() {
        if (buf_count_) {
            release();
        }

        buf_count_ += 1;

        br_mask_ = io_uring_buf_ring_mask(config::N_BUFFERS * buf_count_);

        buffer_.resize(config::N_BUFFERS * config::BUFFER_RING_SIZE * buf_count_);

        char* ptr = buffer_.data();

        int res{};

        br_ = io_uring_setup_buf_ring(peer_ring_, config::N_BUFFERS * buf_count_, 0, 0, &res);

        if (res < 0) {
            throw std::runtime_error("Buffer extend error.");
            std::exit(1);
        }

        for (size_t i{}; i < config::N_BUFFERS * buf_count_; ++i) {
            io_uring_buf_ring_add(br_, ptr, config::BUFFER_RING_SIZE, i, br_mask_, i);
            ptr += config::BUFFER_RING_SIZE;
        }

        io_uring_buf_ring_advance(br_, config::N_BUFFERS * buf_count_);
    }

private:
    void release() {
        if (br_ != nullptr) {
            io_uring_free_buf_ring(peer_ring_, br_, config::N_BUFFERS * buf_count_, 0);
            br_ = nullptr;
        }
    }

private:
    int buf_count_{};

    io_uring* peer_ring_{ nullptr };

    io_uring_buf_ring* br_{ nullptr };

    int br_mask_{};

    std::vector<char> buffer_;
};

}  // namespace net::io
