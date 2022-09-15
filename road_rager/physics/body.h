#ifndef __BODY_H__
#define __BODY_H__

#include "color.h"
#include "list.h"
#include "polygon.h"
#include "vector.h"
#include <stdbool.h>
#include "physics_constants.h"

/**
 * A rigid body constrained to the plane.
 * Implemented as a polygon with uniform density.
 * Bodies can accumulate forces and impulses during each tick.
 * Angular physics (i.e. torques) are not currently implemented.
 */
typedef struct body body_t;

body_type_t *make_type_info(body_type_t type);

/**
 * Initializes a body without any info.
 * Acts like body_init_with_info() where info and info_freer are NULL.
 */
body_t *body_init(list_t *shape, double mass, rgba_color_t color);

/**
 * Updates a body's shape.
 */
void body_update_shape(body_t *body, list_t *new_shape);

/**
 * Sets a body's bounding box.
 */
void body_set_bounding_rect(body_t *body, list_t *bounding_rect);

/**
 * @return whether a body has a bounding box.
 */
bool body_has_bounding_rect(body_t *body);

/**
 * @return a body's bounding box
 */
list_t *body_get_bounding_rect(body_t *body);

/**
 * Allocates memory for a body with the given parameters.
 * The body is initially at rest.
 * Asserts that the mass is positive and that the required memory is allocated.
 *
 * @param shape a list of vectors describing the initial shape of the body
 * @param mass the mass of the body (if INFINITY, stops the body from moving)
 * @param color the color of the body, used to draw it on the screen
 * @param info additional information to associate with the body,
 *   e.g. its type if the scene has multiple types of bodies
 * @param info_freer if non-NULL, a function call on the info to free it
 * @return a pointer to the newly allocated body
 */
body_t *body_init_with_info(
	list_t *shape, double mass, rgba_color_t color, void *info, free_func_t info_freer);

/**
 * Releases the memory allocated for a body.
 *
 * @param body a pointer to a body returned from body_init()
 */
void body_free(body_t *body);

/**
 * Gets the pointer to the current shape of the body.
 * Note that modifying the contents of this list will affect the body's actual shape.
 *
 * @param body a pointer to a body returned from body_init()
 * @return a pointer to the list_t that holds the body's shape
 */
list_t *body_get_shape_pointer(body_t *body);

/**
 * Gets the current shape of a body.
 * Returns a newly allocated vector list, which must be list_free()d.
 *
 * @param body a pointer to a body returned from body_init()
 * @return the polygon describing the body's current position
 */
list_t *body_get_shape(body_t *body);

/**
 * Gets the current center of mass of a body.
 * While this could be calculated with polygon_centroid(), that becomes too slow
 * when this function is called thousands of times every tick.
 * Instead, the body should store its current centroid.
 *
 * @param body a pointer to a body returned from body_init()
 * @return the body's center of mass
 */
vector_t body_get_centroid(body_t *body);

/**
 * Gets the current velocity of a body.
 *
 * @param body a pointer to a body returned from body_init()
 * @return the body's velocity vector
 */
vector_t body_get_velocity(body_t *body);


/**
 * Gets the current angular velocity of a body about omega_center.
 *
 * @param body a pointer to a body returned from body_init()
 * @return the body's angular velocity
 */
double body_get_omega(body_t *body);

/**
 * Gets the current center of rotation of the body.
 *
 * @param body a pointer to a body returned from body_init()
 * @return the position vector of the center of rotation of the body
 */
vector_t body_get_omega_center(body_t *body);

/**
 * Gets the current orientation of a body.
 *
 * @param body a pointer to a body returned from body_init()
 * @return a unit vector pointing along the body's orientation
 */
vector_t body_get_orientation(body_t *body);

/**
 * Gets the mass of a body.
 *
 * @param body a pointer to a body returned from body_init()
 * @return the mass passed to body_init(), which must be greater than 0
 */
double body_get_mass(body_t *body);

/**
 * Gets the moment of inertia of a body.
 *
 * @param body a pointer to a body returned from body_init()
 * @return the moment of inertia of the body
 */
double body_get_MOI(body_t *body);

/**
 * Sets the moment of inertia of a body.
 *
 * @param MOI the moment of inertia to use
 */
void body_set_MOI(body_t *body, double MOI);

/**
 * Gets the display color of a body.
 *
 * @param body a pointer to a body returned from body_init()
 * @return the color passed to body_init(), as an (R, G, B) tuple
 */
rgba_color_t body_get_color(body_t *body);

/**
 * Gets the information associated with a body.
 *
 * @param body a pointer to a body returned from body_init()
 * @return the info passed to body_init()
 */
void *body_get_info(body_t *body);

/**
 * Translates a body to a new position.
 * The position is specified by the *absolute* position of the body's center of mass.
 * Also translates the bounding box of a body, if it has one.
 *
 * @param body a pointer to a body returned from body_init()
 * @param x the body's new centroid
 */
void body_set_centroid(body_t *body, vector_t x);

/**
 * Translates a body to a new position.
 * The position is specified by the position of the body's center of mass *relative*
 * to its current position.
 * Also translates the bounding box of a body, if it has one.
 *
 * @param body a pointer to a body returned from body_init()
 * @param delta the relative change in position for the body centroid
 */
void body_set_centroid_rel(body_t *body, vector_t delta);

/**
 * Sets the center of rotation of the body with respect to its centroid.
 * The position is specified relative to the body's center of mass, in the frame of the
 * body.
 *
 * @param body a pointer to a body returned from body_init()
 * @param x the body's new center of rotation
 */
void body_set_omega_center(body_t *body, vector_t omega_center);

/**
 * Changes a body's velocity (the time-derivative of its position).
 *
 * @param body a pointer to a body returned from body_init()
 * @param v the body's new velocity
 */
void body_set_velocity(body_t *body, vector_t v);

/**
 * Changes a body's angular velocity.
 *
 * @param body a pointer to a body returned from body_init()
 * @param omega the body's new angular velocity
 */
void body_set_omega(body_t *body, double omega);

/**
 * Changes a body's orientation in the plane.
 * The body is rotated about omega_center.
 * Note that the angle is *absolute*, not relative to the current orientation.
 *
 * @param body a pointer to a body returned from body_init()
 * @param angle the body's new angle in radians. Positive is counterclockwise.
 */
void body_set_rotation(body_t *body, double angle);

/**
 * Changes a body's orientation in the plane.
 * The body is rotated about omega_center.
 * Note that the angle is relative to the current orientation.
 *
 * @param body a pointer to a body returned from body_init()
 * @param angle to rotate the body by. Positive is counterclockwise.
 * @param point to rotate the body about.
 */
void body_set_rotation_rel(body_t *body, double angle);

/**
 * Gets the current acceleration of a body.
 *
 * @param body a pointer to a body returned from body_init()
 * @return the body's acceleration vector
 */
vector_t body_get_acceleration(body_t *body);

/**
 * Gets the current speed of a body (normed velocity)
 *
 * @param body a pointer to a body returned from body_init()
 * @return the body's speed
 */
double body_get_speed(body_t *body);

/**
 * Gets the current absolute rotation of a body
 *
 * @param body a pointer to a body returned from body_init()
 * @return the body's absolute rotation
 */
double body_get_rotation(body_t *body);

/**
 * Changes a body's acceleration (the time-derivative of its velocity).
 *
 * @param body a pointer to a body returned from body_init()
 * @param a the body's new acceleration
 */
void body_set_acceleration(body_t *body, vector_t a);

/**
 * Determines if one body is fully contained withing another.
 *
 * @param outer a pointer the the outer body
 * @param inner a pointer to the inner body
 * @return Whether @p inner is fully contained withing @p outer
 */
bool body_contains(body_t *outer, body_t *inner);

/**
 * Checks if all points in the body's polygon are ABOVE / LEFT / RIGHT / BELOW
 * a certain value.
 * @param this pointer to a body
 * @param position enum with the relative position to check
 * @param value value to check
 */
bool body_check_rel_pos(body_t *this, rel_pos direction, double value);

/**
 * Returns the distance between the centroid of two bodies.
 *
 * @param one the first body
 * @param two the second body
 * @return The distance the centroid of @p one and @p two.
 */
double body_get_distance(body_t *one, body_t *two);

/**
 * Applies a force to a body over the current tick.
 * If multiple forces are applied in the same tick, they should be added.
 * Should not change the body's position or velocity; see body_tick().
 *
 * @param body a pointer to a body returned from body_init()
 * @param force the force vector to apply
 */
void body_add_force(body_t *body, vector_t force);

/**
 * Applies a torque to a body over the current tick.
 *
 * @param body a pointer to a body returned from body_init()
 * @param torque the torque to apply
 */
void body_add_torque(body_t *body, double torque);

/**
 * Applies an impulse to a body.
 * An impulse causes an instantaneous change in velocity,
 * which is useful for modeling collisions.
 * If multiple impulses are applied in the same tick, they should be added.
 * Should not change the body's position or velocity; see body_tick().
 *
 * @param body a pointer to a body returned from body_init()
 * @param impulse the impulse vector to apply
 */
void body_add_impulse(body_t *body, vector_t impulse);


/**
 * Applies an angular impulse to a body.
 *
 * @param body a pointer to a body returned from body_init()
 * @param impulse the impulse vector to apply
 */
void body_add_angular_impulse(body_t *body, double angular_impulse);

/**
 * Updates the body after a given time interval has elapsed.
 * Sets acceleration and velocity according to the forces and impulses
 * applied to the body during the tick.
 * Also sets the angular acceleration and velocity according to the torques
 * and angular impulses applied tot he body during the tick.
 * Resets the forces, torques and impulses accumulated on the body.
 * 
 * @param body the body to tick
 * @param dt the number of seconds elapsed since the last tick
 */
void body_tick(body_t *body, double dt);

/**
 * Marks a body for removal--future calls to body_is_removed() will return true.
 * Does not free the body.
 * If the body is already marked for removal, does nothing.
 *
 * @param body the body to mark for removal
 */
void body_remove(body_t *body);

/**
 * Returns whether a body has been marked for removal.
 * This function returns false until body_remove() is called on the body,
 * and returns true afterwards.
 *
 * @param body the body to check
 * @return whether body_remove() has been called on the body
 */
bool body_is_removed(body_t *body);

/**
 * Returns the filepath of the image associated with a body.
 *
 * @param body the body whose image filepath to return
 * @return image filepath
 */
const char *body_get_img(body_t *body);

/**
 * Sets the filepath of the image associated with a body.
 *
 * @param body the body whose image filepath to set
 * @param fpath the filepath
 */
void body_set_img(body_t *body, const char *fpath);

/**
 * Sets a generic object representing additional info fields for a body
 *
 * @param body the body whose info to set
 * @param info generic info object to set
 */
void body_set_info(body_t *body, void *info);

#endif // #ifndef __BODY_H__
