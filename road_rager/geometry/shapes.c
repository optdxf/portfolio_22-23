#include "shapes.h"
#include <math.h>
#include <stdlib.h>

list_t *make_rectangle(vector_t center, double width, double height) {
	list_t *shape = list_init(4, free);
	vector_t *bottom_left = malloc(sizeof(vector_t));
	vector_t *top_left = malloc(sizeof(vector_t));
	vector_t *top_right = malloc(sizeof(vector_t));
	vector_t *bottom_right = malloc(sizeof(vector_t));

	*bottom_left = (vector_t){ center.x - width / 2, center.y - height / 2 };
	*top_right = (vector_t){ center.x + width / 2, center.y + height / 2 };
	*top_left = (vector_t){ center.x - width / 2, center.y + height / 2 };
	*bottom_right = (vector_t){ center.x + width / 2, center.y - height / 2 };

	list_add(shape, bottom_right);
	list_add(shape, top_right);
	list_add(shape, top_left);
	list_add(shape, bottom_left);

	return shape;
}

list_t *make_circle(vector_t center, double radius, size_t points) {
	list_t *polygon = list_init(points, free);
	double angle = 0;

	for (size_t i = 0; i < points; i++) {
		vector_t *point = malloc(sizeof(vector_t));
		*point =
			(vector_t){ radius * cos(angle) + center.x, radius * sin(angle) + center.y };
		list_add(polygon, point);
		angle += (2 * M_PI) / points;
	}

	return polygon;
}

list_t *make_polygon(double *vectors, size_t points, double scaling) {
	list_t *polygon = list_init(points, free);
	for (size_t i = 0; i < points; i++) {
		vector_t *point = malloc(sizeof(vector_t));
		*point = (vector_t){ *(vectors + i * 2) * scaling,
			*((vectors + i * 2) + 1) * scaling };
		list_add(polygon, point);
	}

	return polygon;
}
