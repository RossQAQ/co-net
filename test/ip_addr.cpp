#include "co_net/net/ip_addr.hpp"

#include "co_net/dump.hpp"

using tools::debug::Dump;

int main() {
    auto raw_ip = net::ip::Ipv4Addr::parse("60.205.141.97");

    Dump(), raw_ip.to_string();

    auto sock_addr = net::ip::make_addr_v4("60.205.141.97", 20589);

    Dump(), sock_addr.to_string();

    auto raw_ip6 = net::ip::Ipv6Addr::parse("2001:3CA1:010F:001A:121B:0000:0000:0010");

    Dump(), raw_ip6.to_string();

    auto sock_addr6 = net::ip::make_addr_v6("2001:3CA1:010F:001A:121B:0000:0000:0010", 20589);

    Dump(), sock_addr6.to_string();
}