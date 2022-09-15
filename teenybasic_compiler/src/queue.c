#include "queue.h"
#include <assert.h>
#include <stdio.h>

typedef struct qnode {
	void *value;
	struct qnode *prev;
	struct qnode *next;
} qnode_t;

struct queue {
	size_t size;
	qnode_t *front;
	qnode_t *back;
	free_func_t free_func;
};

queue_t *queue_new(free_func_t free_func) {
	queue_t *q = calloc(1, sizeof(*q));
	q->free_func = free_func;
	assert(q != NULL);
	return q;
}
qnode_t *qnode_new(void *val, qnode_t *prev, qnode_t *next) {
	qnode_t *n = malloc(sizeof(*n));
	assert(n != NULL);
	n->value = val;
	n->prev = prev;
	n->next = next;
	return n;
}
void qnode_free(qnode_t *n, free_func_t free_func) {
	if (free_func) {
		free_func(n->value);
	}
	free(n);
}
void queue_add_to_front(queue_t *q, void *v) {
	qnode_t *n = qnode_new(v, NULL, q->front);
	if (q->front) {
		q->front->prev = n;
	} else {
		// empty
		q->back = n;
	}
	q->front = n;
	++q->size;
}
void queue_add_to_back(queue_t *q, void *v) {
	qnode_t *n = qnode_new(v, q->back, NULL);
	if (q->back) {
		q->back->next = n;
	} else {
		// empty
		q->front = n;
	}
	q->back = n;
	++q->size;
}
void *queue_front(queue_t *q) {
	assert(q->size > 0);
	return q->front->value;
}
void *queue_back(queue_t *q) {
	assert(q->size > 0);
	return q->back->value;
}
void *queue_remove_from_front(queue_t *q) {
	assert(q->size-- > 0);
	qnode_t *n = q->front;
	void *val = n->value;

	q->front = n->next;
	if (q->size == 0) {
		q->back = NULL;
	}
	qnode_free(n, false);
	return val;
}
void *queue_remove_from_back(queue_t *q) {
	assert(q->size-- > 0);
	qnode_t *n = q->back;
	void *val = n->value;

	q->back = n->prev;
	if (q->size == 0) {
		q->front = NULL;
	}
	qnode_free(n, NULL);
	return val;
}
size_t queue_size(queue_t *q) {
	return q->size;
}
void queue_free(queue_t *q) {
	if (q->size > 0) {
		for (qnode_t *n = q->front; n != NULL; ) {
			qnode_t *tmp = n;
			n = n->next;
			qnode_free(tmp, q->free_func);
		}
	}
	free(q);
}
void queue_traverse_front_to_back(queue_t *q, traversal_func_t func, void *opt_arg) {
	for (qnode_t *n = q->front; n != NULL; n = n->next) {
		func(n->value, opt_arg);
	}
}
void queue_traverse_back_to_front(queue_t *q, traversal_func_t func, void *opt_arg) {
	for (qnode_t *n = q->back; n != NULL; n = n->prev) {
		func(n->value, opt_arg);
	}
}