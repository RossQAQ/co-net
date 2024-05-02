#pragma once

#include <liburing.h>

#include <mutex>
#include <thread>
#include <vector>

#include "co_net/config.hpp"
#include "co_net/context/task_loop.hpp"
#include "co_net/context/this_ctx.hpp"
#include "co_net/io/uring.hpp"
#include "co_net/util/noncopyable.hpp"

namespace net::context {

class Context;

using thread_t = std::invoke_result_t<decltype(std::thread::hardware_concurrency)>;

class Workers : public tools::Noncopyable {
public:
    explicit Workers(thread_t how_many, int main_ring_fd, std::latch& init_latch) : main_ring_fd_(main_ring_fd) {
        if (how_many == 0) {
            how_many = std::thread::hardware_concurrency();
        }

        for (size_t i{}; i < how_many; ++i) {
            threads_.emplace_back([&](std::stop_token st) {
                net::context::TaskLoop task_loop;

                net::io::Uring worker_ring{ main_ring_fd_, &task_loop };

                net::io::RingBuffer worker_buffer{ worker_ring.get() };

                this_ctx::local_task_queue = &task_loop;

                this_ctx::local_uring_loop = &worker_ring;

                this_ctx::local_ring_buffer = &worker_buffer;

                {
                    std::lock_guard lg(mtx_);
                    uring_fds_.push_back(worker_ring.fd());
                    init_latch.count_down();
                }

                for (;;) {
                    task_loop.run();

                    worker_ring.submit_all();

                    worker_ring.wait_cqe_arrival();

                    worker_ring.process_batch();

                    if (st.stop_requested()) {
                        return;
                    }
                }
            });
        }
    }

    ~Workers() { stop(); }

    void stop() {
        for (auto& th : threads_) { th.request_stop(); }
    }

    const int get_uring_id_by_index(int idx) const { return uring_fds_.at(idx); }

    std::span<int> get_worker_ids() noexcept { return uring_fds_; }

private:
    std::vector<std::jthread> threads_;

    std::vector<int> uring_fds_;

    int main_ring_fd_;

    std::mutex mtx_;
};

}  // namespace net::context