#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Similar to list.c; a collection of functions that act on a (FIFO) queue.
 * 
 */
typedef struct queue queue_t;

typedef void (*free_func_t)(void *);

queue_t *queue_init(free_func_t freer);
void queue_free(queue_t *q);
size_t queue_size(queue_t *q);
bool queue_empty(queue_t *q);
void *queue_front(queue_t *q);
void *queue_back(queue_t *q);
void queue_enqueue(queue_t *q, void *value);
void *queue_dequeue(queue_t *queue);

#endif