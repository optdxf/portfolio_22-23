#ifndef __POLYGON_H__
#define __POLYGON_H__

#include "list.h"
#include "stdbool.h"
#include "vector.h"

/**
 * Specifies the direction by which the polygon point checking is applied.
 * For example, ABOVE signals that we wish to check if all points of the polygon
 * are above a certain threshold.
 */
typedef enum { ABOVE, BELOW, LEFT, RIGHT } rel_pos;

/**
 * Computes the area of a polygon.
 * See https://en.wikipedia.org/wiki/Shoelace_formula#Statement.
 *
 * @param polygon the list of vertices that make up the polygon,
 * listed in a counterclockwise direction. There is an edge between
 * each pair of consecutive vertices, plus one between the first and last.
 * @return the area of the polygon
 */
double polygon_area(list_t *polygon);

/**
 * Computes the center of mass of a polygon.
 * See https://en.wikipedia.org/wiki/Centroid#Of_a_polygon.
 *
 * @param polygon the list of vertices that make up the polygon,
 * listed in a counterclockwise direction. There is an edge between
 * each pair of consecutive vertices, plus one between the first and last.
 * @return the centroid of the polygon
 */
vector_t polygon_centroid(list_t *polygon);

/**
 * Translates all vertices in a polygon by a given vector.
 * Note: mutates the original polygon.
 *
 * @param polygon the list of vertices that make up the polygon
 * @param translation the vector to add to each vertex's position
 */
void polygon_translate(list_t *polygon, vector_t translation);

/**
 * Rotates vertices in a polygon by a given angle about a given point.
 * Note: mutates the original polygon.
 *
 * @param polygon the list of vertices that make up the polygon
 * @param angle the angle to rotate the polygon, in radians.
 * A positive angle means counterclockwise.
 * @param point the point to rotate around
 */
void polygon_rotate(list_t *polygon, double angle, vector_t point);

/**
 * Checks if a polygon is fully contained within another polygon.
 * @param polygon_out - supposed "outer" polygon
 * @param polygon_in - supposed "inner" polygon
 * @return boolean for whether polygon_in is contained in polygon_out
 */
bool polygon_contain(list_t *polygon_out, list_t *polygon_in);

/**
 * Checks if all points in the polygon are ABOVE / LEFT / RIGHT / BELOW
 * a certain value.
 * @param polygon list_t with the points in the polygon
 * @param position enum with the relative position to check
 * @param value value to check
 */
bool polygon_check_rel_pos(list_t *polygon, rel_pos position, double value);

#endif // #ifndef __POLYGON_H__
