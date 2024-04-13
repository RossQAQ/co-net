#pragma once

#include <thread>
#include <vector>

namespace net::context {

using thread_t = std::invoke_result_t<decltype(std::thread::hardware_concurrency)>;

class ThreadPool {
public:
    ThreadPool(thread_t n = 0) {}

private:
    std::vector<std::jthread> worker_;
};

}  // namespace net::context
