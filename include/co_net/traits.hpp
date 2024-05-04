#pragma once

#include <concepts>
#include <type_traits>

namespace net {

class DirectSocket;

}  // namespace net

namespace net::tcp {

class TcpConnection;

}  // namespace net::tcp

namespace net::traits {

template <typename... Ts>
concept HasDirectFd = requires(Ts...) {
    requires std::disjunction_v<std::is_same<net::DirectSocket, std::remove_cvref_t<Ts>>...> ||
                 std::disjunction_v<std::is_same<net::tcp::TcpConnection, std::remove_cvref_t<Ts>>...>;
};

template <typename... Ts>
constexpr bool has_direct_fd_v = HasDirectFd<Ts...>;
}  // namespace net::traits
