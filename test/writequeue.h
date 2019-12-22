#ifndef WRITE_QUEUE_H
#define WRITE_QUEUE_H

#include <stddef.h>

struct write_req {
    void *buffer;
    size_t length;
    size_t nwrite;
    struct write_req *next;
};

struct write_queue {
    struct write_req *head;
    struct write_req *tail;
};

struct write_queue* write_queue_new(void);

void write_queue_push(struct write_queue *queue, void *buffer, size_t length);

struct write_req* write_queue_peek(struct write_queue *queue);

struct write_req* write_queue_next(struct write_queue *queue);

void write_queue_free(struct write_queue *queue);

#endif
