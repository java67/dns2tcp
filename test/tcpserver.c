#define _GNU_SOURCE
#include "writequeue.h"
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#undef _GNU_SOURCE

int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("usage: %s <bind_ip> <bind_port> <is_reuse_addr> <is_reuse_port>\n", argv[0]);
        return 1;
    }
    return 0;
}
