#include "collision.h"
#include "math_util.h"
#include "vector.h"
#include <float.h>
#include <math.h>

#include <stdio.h>

collision_info_t find_collision(list_t *shape1, list_t *shape2) {
	overlap_axis_t collision1 = find_collision_ordered(shape1, shape2);
	overlap_axis_t collision2 = find_collision_ordered(shape2, shape1);
	vector_t min_axis =
		collision1.overlap > collision2.overlap ? collision2.axis : collision1.axis;

	return (
		collision_info_t){ collision1.overlapping && collision2.overlapping, min_axis };
}

overlap_axis_t find_collision_ordered(list_t *shape1, list_t *shape2) {
	overlap_axis_t axis = { false, (vector_t){ 0, 0 }, 0.0 };
	double min_intersection = DBL_MAX;
	vector_t min_intersect_axis = (vector_t){ 0, 0 };
	size_t num_edges = list_size(shape1);
	vector_t prev_edge = *(vector_t *)list_get(shape1, 0);

	for (size_t i = 0; i < num_edges; i++) {
		vector_t edge = *(vector_t *)list_get(shape1, (i + 1) % num_edges);
		vector_t edge_vec = vec_subtract(edge, prev_edge);
		vector_t perp_vec = vec_unit_vec(vec_rotate(edge_vec, M_PI / 2));

		interval_t shape1interval = project_polygon(shape1, perp_vec);
		interval_t shape2interval = project_polygon(shape2, perp_vec);

		double interval_param = intervals_intersect(shape1interval, shape2interval);

		if (interval_param < 0) {
			axis.overlapping = false;
			return axis;
		} else {
			if (interval_param < min_intersection) {
				min_intersection = interval_param;
				min_intersect_axis = perp_vec;
			}
		}

		prev_edge = edge;
	}
	axis.overlapping = true;
	axis.axis = min_intersect_axis;
	axis.overlap = min_intersection;

	return axis;
}

interval_t project_polygon(list_t *shape, vector_t axis) {
	double min = DBL_MAX;
	double max = -DBL_MAX;
	for (size_t i = 0; i < list_size(shape); i++) {
		double dot = vec_dot(*(vector_t *)list_get(shape, i), axis);
		min = min_dbl(min, dot);
		max = max_dbl(max, dot);
	}
	return (interval_t){ min, max };
}

double intervals_intersect(interval_t interval1, interval_t interval2) {
	if (interval1.min < interval2.max ? interval1.max > interval2.min
									  : interval2.max > interval1.min) {
		return interval1.min < interval2.max ? interval2.max - interval1.min
											 : interval1.max - interval2.min;
	} else {
		return -1;
	}
}