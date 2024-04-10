#pragma once

#include <liburing.h>

namespace net::time::literals {

using ull = unsigned long long;

__kernel_timespec operator""s(ull second) {
    return { static_cast<long long>(second), 0 };
}

__kernel_timespec operator""ms(ull ms) {
    long long sec = ms / 1000;
    ms -= sec * 1000;
    ms *= 1'000'000;
    return { sec, static_cast<long long>(ms) };
}

__kernel_timespec operator""us(ull us) {
    long long sec = us / 1'000'000;
    us -= sec * 1'000'000;
    us *= 1'000;
    return { sec, static_cast<long long>(us) };
}

__kernel_timespec operator""ns(ull ns) {
    long long sec = ns / 1'000'000'000;
    ns -= sec * 1'000'000'000;
    return { sec, static_cast<long long>(ns) };
}

}  // namespace net::time::literals
