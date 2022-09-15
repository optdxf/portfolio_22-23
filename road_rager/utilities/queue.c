#include "queue.h"

typedef struct queue_item {
	void *data;
	void *next;
} queue_item_t;

struct queue {
	size_t size;
	queue_item_t *front;
	queue_item_t *back;
	free_func_t freer;
};

queue_t *queue_init(free_func_t freer) {
	queue_t *q = malloc(sizeof(*q));
	assert("could not allocate memory" && q);

	q->front = NULL;
	q->back = NULL;
	q->freer = freer;
	q->size = 0;
	return q;
}
void queue_free(queue_t *q) {
	while (q->front) {
		queue_item_t *element = q->front;
		q->front = element->next;

		if (q->freer && element->data) {
			q->freer(element->data);
		}
		free(element);
	}
	free(q);
}
size_t queue_size(queue_t *q) { return q->size; }
bool queue_empty(queue_t *q) { return !q->size; }
void *queue_front(queue_t *q) { return q->front ? q->front->data : NULL; }
void *queue_back(queue_t *q) { return q->back ? q->back->data : NULL; }
void queue_enqueue(queue_t *q, void *value) {
	queue_item_t *element = malloc(sizeof(*element));
	element->data = value;
	element->next = NULL;
	if (!q->size) {
		q->front = element;
		q->back = element;
	} else {
		q->back->next = element;
	}
	q->size++;
}
void *queue_dequeue(queue_t *q) {
	if (q->front) {
		queue_item_t *element = q->front;
		void *data = element->data;
		q->front = element->next;
		if (q->front == NULL) {
			// this was the only element
			q->back = NULL;
		}
		q->size--;
		free(element);
		return data;
	} else {
		return NULL;
	}
}