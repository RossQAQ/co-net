#pragma once

#include <liburing.h>

namespace net::impl {

io_uring_sqe* impl_mshot_accept_send_fd(int direct_fd);

};  // namespace net::impl