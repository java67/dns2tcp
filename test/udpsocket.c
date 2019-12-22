#define _GNU_SOURCE
#include <ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#undef _GNU_SOURCE

static int g_sockfd = 0;

#define STDIN_BUFLEN 1024
#define SOCKET_BUFLEN 1024
static char g_stdin_buffer[STDIN_BUFLEN] = {0};
static char g_socket_buffer[SOCKET_BUFLEN] = {0};

static void stdin_read_cb(struct ev_loop *loop, struct ev_io *stdin_watcher, int events) {
    ssize_t nread = read(STDIN_FILENO, g_stdin_buffer, STDIN_BUFLEN);
    if (nread < 0) {
        perror("failed to read from stdin");
        exit(errno);
    }
    if (nread == 0) {
        // EOF
        exit(0);
    }
    g_stdin_buffer[nread] = 0;

    /* peer_ip peer_port msg_string */
    char *peer_ipstr = NULL;
    uint16_t peer_portno = 0;
    char *msg_string = NULL;
    for (char *token = strtok(g_stdin_buffer, " "), cnt = 0; token && cnt < 3; token = strtok(NULL, " "), ++cnt) {
        switch (cnt) {
            case 0:
                peer_ipstr = token;
                break;
            case 1:
                peer_portno = strtol(token, NULL, 10);
                break;
            case 2:
                msg_string = token;
                break;
        }
    }
    if (!(peer_ipstr && peer_portno && msg_string)) {
        printf("input format error: <peer_ip> <peer_port> <msg_string>\n");
        return;
    }

    struct sockaddr_in peeraddr = {0};
    peeraddr.sin_family = AF_INET;
    inet_pton(AF_INET, peer_ipstr, &peeraddr.sin_addr);
    peeraddr.sin_port = htons(peer_portno);

    if (sendto(g_sockfd, msg_string, strlen(msg_string), 0, (void *)&peeraddr, sizeof(peeraddr)) < 0) {
        perror("failed to send message to peer");
    }
    printf("[send-msg] to:%s#%hu, msg:%s", peer_ipstr, peer_portno, msg_string);
}

static void socket_read_cb(struct ev_loop *loop, struct ev_io *socket_watcher, int events) {
    struct sockaddr_in peeraddr = {0};
    ssize_t nread = recvfrom(g_sockfd, g_socket_buffer, SOCKET_BUFLEN, 0, (void *)&peeraddr, &(socklen_t){sizeof(peeraddr)});
    if (nread < 0) {
        perror("failed to recv message from peer");
        return;
    }
    if (nread == 0) return;
    g_socket_buffer[nread] = 0;

    char peer_ipstr[INET6_ADDRSTRLEN] = {0};
    uint16_t peer_portno = ntohs(peeraddr.sin_port);
    inet_ntop(AF_INET, &peeraddr.sin_addr, peer_ipstr, INET6_ADDRSTRLEN);

    printf("[recv-msg] from:%s#%hu, msg:%s", peer_ipstr, peer_portno, g_socket_buffer);
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("usage: %s <bind_ip> <bind_port> <is_reuse_addr> <is_reuse_port>\n", argv[0]);
        return 1;
    }
    char *bind_ipstr = argv[1];
    uint16_t bind_portno = strtol(argv[2], NULL, 0);
    bool is_reuse_addr = strcmp(argv[3], "true") == 0;
    bool is_reuse_port = strcmp(argv[4], "true") == 0;
    printf("=====================================\n");
    printf("bind address: %s#%hu\n", bind_ipstr, bind_portno);
    printf("is_reuse_addr: %s\n", is_reuse_addr ? "true" : "false");
    printf("is_reuse_port: %s\n", is_reuse_port ? "true" : "false");
    printf("=====================================\n");

    struct ev_loop *loop = ev_default_loop(0);

    struct ev_io *stdin_watcher = &(struct ev_io){0};
    ev_io_init(stdin_watcher, stdin_read_cb, STDIN_FILENO, EV_READ);
    ev_io_start(loop, stdin_watcher);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("failed to create socket");
        return errno;
    }
    g_sockfd = sockfd;

    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        perror("failed to get socket flags");
        return errno;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("failed to set socket flags");
        return errno;
    }

    if (is_reuse_addr && setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("failed to setsockopt(SO_REUSEADDR)");
        return errno;
    }
    if (is_reuse_port && setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int)) < 0) {
        perror("failed to setsockopt(SO_REUSEPORT)");
        return errno;
    }

    struct sockaddr_in skaddr = {0};
    skaddr.sin_family = AF_INET;
    inet_pton(AF_INET, bind_ipstr, &skaddr.sin_addr);
    skaddr.sin_port = htons(bind_portno);
    if (bind(sockfd, (void *)&skaddr, sizeof(skaddr)) < 0) {
        perror("failed to bind address");
        return errno;
    }

    struct ev_io *socket_watcher = &(struct ev_io){0};
    ev_io_init(socket_watcher, socket_read_cb, sockfd, EV_READ);
    ev_io_start(loop, socket_watcher);

    ev_run(loop, 0);
    return 0;
}
