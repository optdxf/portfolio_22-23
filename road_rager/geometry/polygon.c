#include "polygon.h"

double polygon_area(list_t *polygon) {
	double sum = 0;
	for (size_t idx = 0; idx < list_size(polygon); ++idx) {
		if (idx == list_size(polygon) - 1) {
			// for the last vector, we need to take the cross product w/ the first
			// vector, per the formula
			sum += vec_cross(
				*(vector_t *)list_get(polygon, idx), *(vector_t *)list_get(polygon, 0));
		} else {
			sum += vec_cross(*(vector_t *)list_get(polygon, idx),
				*(vector_t *)list_get(polygon, idx + 1));
		}
	}
	return sum / 2;
}

vector_t polygon_centroid(list_t *polygon) {
	vector_t center = {};
	for (size_t idx = 0; idx < list_size(polygon); ++idx) {
		vector_t *vec1 = (vector_t *)list_get(polygon, idx);
		// ternary operator b/c of special case where last vec is paired w/ first
		vector_t *vec2 =
			(vector_t *)list_get(polygon, idx == list_size(polygon) - 1 ? 0 : idx + 1);

		center.x += (vec1->x + vec2->x) * vec_cross(*vec1, *vec2);
		center.y += (vec1->y + vec2->y) * vec_cross(*vec1, *vec2);
	}
	return vec_multiply(1 / (6 * polygon_area(polygon)), center);
}

void polygon_translate(list_t *polygon, vector_t translation) {
	for (size_t i = 0; i < list_size(polygon); ++i) {
		vector_t *vec = (vector_t *)list_get(polygon, i);
		*vec = vec_add(*vec, translation);
	}
}

void polygon_rotate(list_t *polygon, double angle, vector_t point) {
	for (size_t i = 0; i < list_size(polygon); i++) {
		vector_t *vec = (vector_t *)list_get(polygon, i);
		// make point origin, rotate, and add point back
		*vec = vec_add(vec_rotate(vec_subtract(*vec, point), angle), point);
	}
}

bool polygon_contain(list_t *polygon_out, list_t *polygon_in) {
	// nvert:  Number of vertices in the polygon.
	// vertx, verty: Arrays containing the x- and y-coordinates of the polygon's
	// vertices. testx, testy: X- and y-coordinate of the test point.

	// implement Point-In-Polygon algorithm
	for (size_t k = 0; k < list_size(polygon_in); k++) {
		bool point_contain = false;

		double testx = ((vector_t *)list_get(polygon_in, k))->x;
		double testy = ((vector_t *)list_get(polygon_in, k))->y;

		for (size_t i = 0, j = list_size(polygon_out) - 1; i < list_size(polygon_out);
			 j = i++) {
			double vertx_i = ((vector_t *)list_get(polygon_out, i))->x;
			double verty_i = ((vector_t *)list_get(polygon_out, i))->y;

			double vertx_j = ((vector_t *)list_get(polygon_out, j))->x;
			double verty_j = ((vector_t *)list_get(polygon_out, j))->y;

			if (((verty_i > testy) != (verty_j > testy)) &&
				(testx < (vertx_j - vertx_i) * (testy - verty_i) / (verty_j - verty_i) +
							 vertx_i)) {
				point_contain = !point_contain;
			}
		}
		if (!point_contain) {
			return false;
		}
	}

	return true;
}

bool polygon_check_rel_pos(list_t *polygon, rel_pos position, double value) {
	for (size_t i = 0; i < list_size(polygon); i++) {
		switch (position) {
			case ABOVE: {
				if (((vector_t *)list_get(polygon, i))->y <= value) {
					return false;
				}
				break;
			}
			case BELOW: {
				if (((vector_t *)list_get(polygon, i))->y >= value) {
					return false;
				}
				break;
			}
			case LEFT: {
				if (((vector_t *)list_get(polygon, i))->x >= value) {
					return false;
				}
				break;
			}
			case RIGHT: {
				if (((vector_t *)list_get(polygon, i))->x <= value) {
					return false;
				}
				break;
			}
		}
	}

	return true;
}
