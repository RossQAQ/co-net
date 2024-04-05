/**
 * @file noncopyable.hpp
 * @author Roseveknif (rossqaq@outlook.com)
 * @brief 禁止拷贝的工具类。
 * @version 0.1
 * @date 2024-03-29
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

namespace tools {

struct Noncopyable {
protected:
    Noncopyable() = default;
    ~Noncopyable() = default;

private:
    Noncopyable(const Noncopyable&) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;
};

}  // namespace tools
