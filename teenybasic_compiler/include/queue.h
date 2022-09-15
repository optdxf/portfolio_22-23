#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct queue queue_t;
typedef void (*free_func_t)(void*);
typedef void (*traversal_func_t)(void*, void*);

queue_t *queue_new(free_func_t free_func);
void queue_add_to_front(queue_t *q, void *v);
void queue_add_to_back(queue_t *q, void *v);
void *queue_front(queue_t *q);
void *queue_back(queue_t *q);
void *queue_remove_from_front(queue_t *q);
void *queue_remove_from_back(queue_t *q);
size_t queue_size(queue_t *q);
void queue_free(queue_t *q);
void queue_traverse_front_to_back(queue_t *q, traversal_func_t func, void *opt_arg);
void queue_traverse_back_to_front(queue_t *q, traversal_func_t func, void *opt_arg);

#endif