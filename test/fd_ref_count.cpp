#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "co_net/dump.hpp"

void fd_ref(int fd) {
    sleep(999999);
}

int main() {
    int fd = ::open("/home/ecs-user/co-net/sample_text/one_sentence", O_RDONLY);
    fd_ref(fd);
}