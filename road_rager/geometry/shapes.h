// Collection of helper functions to generate generic shapes
#ifndef __SHAPES_H__
#define __SHAPES_H__

#include "list.h"
#include "vector.h"

/**
* @param center the center vector
* @param width rectangle width
* @param height rectangle height
* @return list of vectors
*/
list_t *make_rectangle(vector_t center, double width, double height);

/**
* @param center the center vector
* @param radius circle radius
* @param points resolution of the circle
* @return list of vectors
*/
list_t *make_circle(vector_t center, double radius, size_t points);

/**
* @param vectors a list of vectors (2d array of coordinates)
* @param points number of points
* @param scaling scaling coefficient
* @return list of vectors
*/
list_t *make_polygon(double *vectors, size_t points, double scaling);

#endif