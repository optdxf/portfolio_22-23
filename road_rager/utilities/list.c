#include "list.h"
#include <assert.h>
#include <stdlib.h>

struct list {
	size_t size;
	size_t capacity;
	void **data;
	free_func_t freer;
};

list_t *list_init(size_t initial_size, free_func_t freer) {
	list_t *new_list = malloc(sizeof(list_t));
	assert("could not allocate memory" && new_list != NULL);

	new_list->size = 0;
	new_list->capacity = initial_size;
	new_list->freer = freer;
	new_list->data = malloc(sizeof(void *) * initial_size);
	assert("could not allocate memory" && new_list->data != NULL);

	return new_list;
}

void list_free(list_t *list) {
	if (list->freer) {
		for (size_t i = 0; i < list->size; i++) {
			list->freer(list->data[i]);
		}
	}
	free(list->data);
	free(list);
}

size_t list_size(list_t *list) { return list->size; }

void *list_get(list_t *list, size_t index) {
	assert("index out of bounds" && index < list->size);
	return list->data[index];
}

void list_add(list_t *list, void *value) {
	assert("value is NULL" && value != NULL);
	if (list->size < list->capacity) {
		list->data[list->size++] = value;
	} else {
		list->capacity++; // increment before multiplying in the case where capacity
						  // is 0
		list->capacity *= 2;
		list->data = realloc(list->data, sizeof(void *) * list->capacity);
		assert("realloc failed; could not resize list" && list->data != NULL);
		list->data[list->size] = value;
		list->size++;
	}
}

void *list_remove(list_t *list, size_t idx) {
	assert("list is empty" && list->size > 0);
	assert("index out of bounds" && idx < list->size);

	void *temp = list->data[idx];
	list->size--;

	while (idx < list->size) {
		list->data[idx] = list->data[idx + 1];
		++idx;
	}

	return temp;
}