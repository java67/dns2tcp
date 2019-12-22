#include "writequeue.h"
#include <stdlib.h>

struct write_queue* write_queue_new(void) {
    return calloc(1, sizeof(struct write_queue));
}

void write_queue_push(struct write_queue *queue, void *buffer, size_t length) {
    struct write_req *req = calloc(1, sizeof(struct write_req));
    req->buffer = buffer;
    req->length = length;
    if (!queue->head && !queue->tail) {
        queue->head = req;
        queue->tail = req;
    } else {
        queue->tail->next = req;
        queue->tail = req;
    }
}

struct write_req* write_queue_peek(struct write_queue *queue) {
    return queue->head;
}

struct write_req* write_queue_next(struct write_queue *queue) {
    struct write_req *req = queue->head;
    if (!req) return NULL;
    if (queue->head == queue->tail) {
        queue->head = NULL;
        queue->tail = NULL;
    } else {
        queue->head = req->next;
    }
    free(req->buffer);
    free(req);
    return queue->head;
}

void write_queue_free(struct write_queue *queue) {
    for (struct write_req *req = queue->head, *tmpreq = NULL; req; req = tmpreq) {
        tmpreq = req->next;
        free(req->buffer);
        free(req);
    }
    free(queue);
}
