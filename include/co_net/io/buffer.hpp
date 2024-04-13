#pragma once

#include <liburing.h>

#include <vector>

#include "co_net/config.hpp"

namespace net::io {

class ProvidedBuffer {
public:
    ProvidedBuffer(int group_id) : group_id_(group_id) { buffer_.resize(::config::PROVIDED_BUFFER_INITIAL_SIZE); }

    void expand() {
        auto sz = buffer_.size();
        buffer_.resize(sz * 2);
    }

    int block() const noexcept { return buffer_.size() / buffer_slice_; }

    void* address() noexcept { return reinterpret_cast<void*>(buffer_.data()); }

private:
    const size_t buffer_slice_{ 1024 };
    int group_id_{};

    std::vector<char> buffer_;
};

}  // namespace net::io
