#ifndef __COLLISION_H__
#define __COLLISION_H__

#include "list.h"
#include "vector.h"
#include <stdbool.h>

typedef struct interval {
	double min;
	double max;
} interval_t;

/**
 * Represents the status of a collision between two shapes.
 * The shapes are either not colliding, or they are colliding along some axis.
 */
typedef struct overlap_axis {
	bool overlapping;
	vector_t axis;
	double overlap;
} overlap_axis_t;

/**
 * Represents the status of a collision between two shapes.
 * The shapes are either not colliding, or they are colliding along some axis.
 */
typedef struct {
	/** Whether the two shapes are colliding */
	bool collided;
	/**
	 * If the shapes are colliding, the axis they are colliding on.
	 * This is a unit vector pointing from the first shape towards the second.
	 * Normal impulses are applied along this axis.
	 * If collided is false, this value is undefined.
	 */
	vector_t axis;
} collision_info_t;

/**
 * Computes the status of the collision between two convex polygons.
 * The shapes are given as lists of vertices in counterclockwise order.
 * There is an edge between each pair of consecutive vertices,
 * and one between the first vertex and the last vertex.
 *
 * @param shape1 the first shape
 * @param shape2 the second shape
 * @return whether the shapes are colliding, and if so, the collision axis.
 * The axis should be a unit vector pointing from shape1 towards shape2.
 */
collision_info_t find_collision(list_t *shape1, list_t *shape2);

/**
 * Determines whether two convex polygons intersect with the projection method,
 * using the axes from shape 1 only.
 * @param shape1 the first shape
 * @param shape2 the second shape
 * @return overlap_axis_t with the information about whether the axes are overlapping
 */
overlap_axis_t find_collision_ordered(list_t *shape1, list_t *shape2);

/**
 * Returns an interval containing the min and max of the polygon
 * projected onto the axis
 * @param shape the polygon in question
 * @param axis unit vector of the axis to project onto
 * @param return interval_t containing min and max of the vertex projections
 */
interval_t project_polygon(list_t *shape, vector_t axis);

/**
 * Returns -1 if the two intervals do not intersect and their intersecting amount
 * otherwise
 */
double intervals_intersect(interval_t interval1, interval_t interval2);

#endif // #ifndef __COLLISION_H__
