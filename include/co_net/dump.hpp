#pragma once

#include <cstring>
#include <format>
#include <iostream>
#include <ranges>
#include <source_location>
#include <string_view>
#include <vector>

namespace tools::debug {

#ifndef DEBUG_NO_FUNC_NAME
    #define SHOW true
#else
    #define SHOW false
#endif

enum class Color { black = 30, red, green, yellow, blue, magenta, cyan, white };

class Dump {
public:
    explicit Dump(Color color = Color::white,
                  bool func_name = SHOW,
                  const std::source_location& loc = std::source_location::current()) noexcept :
        loc_(loc),
        color_(color) {
        std::string file_name = loc_.file_name();
        auto v = file_name | std::views::split('/') |
                 std::views::transform([](auto word) { return std::string(word.begin(), word.end()); });
        std::vector<std::string> words(v.begin(), v.end());
        formatted_ = std::format("\033[1;31m{}\033[0m: \033[1;36m{}\033[0m, \033[1;35m{}\033[0m\n",
                                 *words.rbegin(),
                                 loc_.line(),
                                 func_name ? loc_.function_name() : "");
    }
    ~Dump() { print(); }

    Dump(const Dump&) = delete;
    Dump& operator=(const Dump&) = delete;

    template <typename T>
    Dump& operator,(T&& msg) {
        formatted_ = std::format("{}\033[1;{}m{}\033[0m\n", formatted_, static_cast<int>(color_), msg);
        return *this;
    }

    template <typename T>
        requires(std::is_convertible_v<T, std::string> && !std::is_same_v<T, std::string>)
    Dump& operator,(T&& msg) {
        std::string msg_str = msg;
        std::string no_transfer;
        for (char ch : msg_str) {
            switch (ch) {
                case '\n':
                    no_transfer.append("\\n");
                    break;

                default:
                    no_transfer.append(1, ch);
                    break;
            }
            if (ch == 0)
                break;
        }

        formatted_ = std::format("{}\033[1;{}m{}\033[0m\n", formatted_, static_cast<int>(color_), no_transfer);
        return *this;
    }

private:
    void print() { std::cout << formatted_ << '\n'; }

private:
    Color color_;
    const std::source_location& loc_;
    std::string formatted_;
};
}  // namespace tools::debug
