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

#define BUFLEN 1024
static struct ev_io       *g_socket_watcher     = NULL;
static struct write_queue *g_socket_write_queue = NULL;
static char                g_socket_buffer[BUFLEN] = {0};

static void stdin_read_cb(struct ev_loop *loop, struct ev_io *watcher, int events) {
    char *buffer = malloc(BUFLEN);
    ssize_t nread = read(STDIN_FILENO, buffer, BUFLEN);
    if (nread < 0) {
        perror("failed to recv from stdin");
        exit(errno);
    }
    if (nread == 0) {
        exit(0);
    }
    buffer[nread] = 0;

    write_queue_push(g_socket_write_queue, buffer, nread);

    if (!(g_socket_watcher->events & EV_WRITE)) {
        ev_io_stop(loop, g_socket_watcher);
        ev_io_set(g_socket_watcher, g_socket_watcher->fd, EV_READ | EV_WRITE);
        ev_io_start(loop, g_socket_watcher);
    }
}

static void socket_event_cb(struct ev_loop *loop, struct ev_io *watcher, int events) {
    int sockfd = watcher->fd;

    if (events & EV_READ) {
        ssize_t nread = recv(sockfd, g_socket_buffer, BUFLEN, 0);
        if (nread < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) goto DO_WRITE;
            perror("failed to recv from peer");
            exit(errno);
        }
        if (nread == 0) {
            printf("peer closed the connection\n");
            exit(0);
        }
        g_socket_buffer[nread] = 0;
        printf("[recv-msg] %s", g_socket_buffer);
    }

DO_WRITE:
    if (events & EV_WRITE) {
        for (struct write_req *req = write_queue_peek(g_socket_write_queue); req; req = write_queue_next(g_socket_write_queue)) {
            while (req->nwrite < req->length) {
                ssize_t nwrite = send(sockfd, req->buffer + req->nwrite, req->length - req->nwrite, 0);
                if (nwrite < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) return;
                    perror("failed to send to peer");
                    exit(errno);
                }
                req->nwrite += nwrite;
            }
            printf("[send-msg] %s", (char *)req->buffer);
        }
        ev_io_stop(loop, watcher);
        ev_io_set(watcher, sockfd, EV_READ);
        ev_io_start(loop, watcher);
    }
}

static void socket_connect_cb(struct ev_loop *loop, struct ev_io *watcher, int events) {
    int sockfd = watcher->fd;
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void *)&errno, &(socklen_t){sizeof(errno)}) < 0 || errno) {
        perror("failed to connect to peer");
        exit(errno);
    }
    ev_io_stop(loop, watcher);
    ev_io_init(watcher, socket_event_cb, sockfd, EV_READ);
    ev_io_start(loop, watcher);
}

int main(int argc, char *argv[]) {
    if (argc < 7) {
        printf("usage: %s <bind_ip> <bind_port> <peer_ip> <peer_port> <is_reuse_addr> <is_reuse_port>\n", argv[0]);
        return 1;
    }
    char *bind_ipstr = argv[1];
    uint16_t bind_portno = strtol(argv[2], NULL, 10);
    char *peer_ipstr = argv[3];
    uint16_t peer_portno = strtol(argv[4], NULL, 10);
    bool is_reuse_addr = strcmp(argv[5], "true") == 0;
    bool is_reuse_port = strcmp(argv[6], "true") == 0;
    printf("=====================================\n");
    printf("bind address: %s#%hu\n", bind_ipstr, bind_portno);
    printf("peer address: %s#%hu\n", peer_ipstr, peer_portno);
    printf("is_reuse_addr: %s\n", is_reuse_addr ? "true" : "false");
    printf("is_reuse_port: %s\n", is_reuse_port ? "true" : "false");
    printf("=====================================\n");

    struct ev_loop *loop = ev_default_loop(0);

    struct ev_io *stdin_watcher = &(struct ev_io){0};
    ev_io_init(stdin_watcher, stdin_read_cb, STDIN_FILENO, EV_READ);
    ev_io_start(loop, stdin_watcher);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("failed to create socket");
        return errno;
    }

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
    
    struct sockaddr_in bind_addr = {0};
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(bind_portno);
    inet_pton(AF_INET, bind_ipstr, &bind_addr.sin_addr);
    if (bind(sockfd, (void *)&bind_addr, sizeof(bind_addr)) < 0) {
        perror("failed to bind address");
        return errno;
    }

    struct sockaddr_in peer_addr = {0};
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(peer_portno);
    inet_pton(AF_INET, peer_ipstr, &peer_addr.sin_addr);
    if (connect(sockfd, (void *)&peer_addr, sizeof(peer_addr)) < 0 && errno != EINPROGRESS) {
        perror("failed to connect to peer");
        return errno;
    }

    struct ev_io *socket_watcher = &(struct ev_io){0};
    ev_io_init(socket_watcher, socket_connect_cb, sockfd, EV_WRITE);
    ev_io_start(loop, socket_watcher);

    g_socket_watcher = socket_watcher;
    g_socket_write_queue = write_queue_new();

    ev_run(loop, 0);
    return 0;
}
