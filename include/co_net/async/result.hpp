/**
 * @file result.hpp
 * @author Roseveknif (rossqaq@outlook.com)
 * @brief Task 协程的返回结果
 * @version 0.1
 * @date 2024-04-05
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <coroutine>
#include <memory>

#include "co_net/util/noncopyable.hpp"

namespace net::async {

template <typename T>
class Result : public tools::Noncopyable {
public:
    Result() {}

    ~Result() noexcept {}

    [[nodiscard]] T move_out_result() {
        T ret(std::move(value_));
        std::destroy_at(std::addressof(value_));
        return ret;
    }

    template <typename... Ts>
    void construct_result(Ts&&... args) {
        std::construct_at(std::addressof(value_), std::forward<T>(args)...);
    }

private:
    union {
        T value_;
    };
};

}  // namespace net::async
