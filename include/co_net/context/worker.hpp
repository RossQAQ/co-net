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
    explicit Workers(thread_t how_many, int main_uring_fd, std::latch& init_latch) {
        uring_id_.push_back(main_uring_fd);

        if (how_many == 0) {
            how_many = std::thread::hardware_concurrency();
        }

        for (size_t i{}; i < how_many; ++i) {
            threads_.emplace_back([&](std::stop_token st) {
                using namespace net::time_literals;

                net::context::TaskLoop task_loop;

                net::io::Uring worker_ring{ net::io::UringType::Minor, &task_loop };

                this_ctx::local_task_queue = &task_loop;

                this_ctx::local_uring_loop = &worker_ring;

                {
                    std::lock_guard lg(mtx_);
                    uring_id_.push_back(worker_ring.fd());
                    init_latch.count_down();
                }

                for (;;) {
                    task_loop.run();

                    auto ret = worker_ring.wait_cqe_arrival_in(::config::MINOR_URING_WAIT_CQE_TIMEOUT);

                    if (ret != -ETIME && ret >= 0) {
                        worker_ring.process_batch();
                    }

                    if (st.stop_requested()) {
                        return;
                    }
                }
            });
        }
    }

    ~Workers() {
        for (std::jthread& th : threads_) { th.request_stop(); }
    }

    const int get_uring_id_by_index(int idx) const { return uring_id_.at(idx); }

private:
    std::vector<std::jthread> threads_;

    std::vector<int> uring_id_;

    std::mutex mtx_;
};

}  // namespace net::context