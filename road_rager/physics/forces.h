#ifndef __FORCES_H__
#define __FORCES_H__

#include "car.h"
#include "scene.h"
#include "client.h"

/**
* Replaces the info field of a body with reference parameters used when resolving
* collisions between objects.
* @param bounding_circle_radius the radius of the circle bounding the object, used for
* ignoring collisions between objects that are far apart
* @param collision_flag whether the object is currently colliding with another object
* @param touch_grass whether the object is current touching grass
* @param orientation_vector vector along which to resolve the collision; overwrites the collision
* vector typically computed in collision.c
* @param body_type the body type of the object (originally the only information stored in body_info)
*/
typedef struct collision_ref {
	double bounding_circle_radius;
	bool collision_flag;
	bool touch_grass;
	vector_t orientation_vector;
	size_t body_type; // body type enum defined in map.h
} collision_ref_t;

/**
 * A function called when a collision occurs.
 * @param body1 the first body passed to create_collision()
 * @param body2 the second body passed to create_collision()
 * @param axis a unit vector pointing from body1 towards body2
 *   that defines the direction the two bodies are colliding in
 * @param aux the auxiliary value passed to create_collision()
 */
typedef void (*collision_handler_t)(
	body_t *body1, body_t *body2, vector_t axis, void *aux);

/**
 * Adds a force creator to a scene that applies gravity between two bodies.
 * The force creator will be called each tick
 * to compute the Newtonian gravitational force between the bodies.
 * See https://en.wikipedia.org/wiki/Newton%27s_law_of_universal_gravitation#Vector_form.
 * The force should not be applied when the bodies are very close,
 * because its magnitude blows up as the distance between the bodies goes to 0.
 *
 * @param scene the scene containing the bodies
 * @param G the gravitational proportionality constant
 * @param body1 the first body
 * @param body2 the second body
 */
void create_newtonian_gravity(scene_t *scene, double G, body_t *body1, body_t *body2);

/**
 * Adds a force creator to a scene that acts like a spring between two bodies.
 * The force creator will be called each tick
 * to compute the Hooke's-Law spring force between the bodies.
 * See https://en.wikipedia.org/wiki/Hooke%27s_law.
 *
 * @param scene the scene containing the bodies
 * @param k the Hooke's constant for the spring
 * @param body1 the first body
 * @param body2 the second body
 */
void create_spring(scene_t *scene, double k, body_t *body1, body_t *body2);

/**
 * Adds a force creator to a scene that applies a drag force on a body.
 * The force creator will be called each tick
 * to compute the drag force on the body proportional to its velocity.
 * The force points opposite the body's velocity.
 *
 * @param scene the scene containing the bodies
 * @param gamma the proportionality constant between force and velocity
 *   (higher gamma means more drag)
 * @param body the body to slow down
 */
void create_drag(scene_t *scene, double gamma, body_t *body);

/**
 * Adds a force creator to a scene that applies a drag force on a car body.
 * The force creator will be called each tick
 * to compute the drag force on the body proportional to its squared velocity.
 * The force points opposite the body's velocity.
 * The drag constant is a pointer so its value can be dynamically changed.
 *
 * @param scene the scene containing the bodies
 * @param gamma pointer to the proportionality constant between force and velocity
 * @param body the car body to slow down
 */
void create_car_drag(scene_t *scene, double *gamma, body_t *body);

/**
 * Adds a force creator to a scene that applies an engine force on a car body.
 * The force creator will be called each tick to compute the engine force on the body,
 * based on the engine power constant.
 * The force points in the direction of the car's orientation.
 * The power constant is a pointer so its value can be dynamically changed.
 *
 * @param scene the scene containing the bodies
 * @param power pointer to the engine power constant
 * @param body the car body to apply the engine force on
 */
void create_dynamic_engine(scene_t *scene, double *power, body_t *body);

/**
 * Adds a force creator to a scene that applies a braking force on a car body.
 * The force creator will be called each tick to compute the braking force on the body,
 * based on the braking power constant.
 * The force points opposite to the direction of the car's orientation.
 * The power constant is a pointer so its value can be dynamically changed.
 *
 * @param scene the scene containing the bodies
 * @param power pointer to the braking power constant
 * @param body the car body to apply the braking force on
 */
void create_dynamic_braking(scene_t *scene, double *power, body_t *body);

/**
 * Adds a force creator to a scene that applies a frictional force on a car body.
 * The force creator will be called each tick to compute the frictional force on the body,
 * based on the frictional coefficient.
 * The force points orthogonally to the car's orientation, such that it opposes relative slip.
 * The frictional coefficient is a pointer so its value can be dynamically changed.
 *
 * @param scene the scene containing the bodies
 * @param mu pointer to the frictional coefficient
 * @param body the car body to apply the frictional force on
 */
void create_friction(scene_t *scene, double *mu, body_t *body);

/**
 * Adds a timed remover to a scene that removes an object after a set time.
 *
 * @param scene the scene containing the bodies
 * @param seconds time to remove object after
 * @param body object to remove
 */
void create_timed_remover(scene_t *scene, double seconds, body_t *body);

/**
 * Adds a force creator to a scene that calls a given collision handler
 * function each time two bodies collide.
 * This generalizes create_destructive_collision() from last week,
 * allowing different things to happen on a collision.
 * The handler is passed the bodies, the collision axis, and an auxiliary value.
 * It should only be called once while the bodies are still colliding.
 *
 * @param scene the scene containing the bodies
 * @param body1 the first body
 * @param body2 the second body
 * @param handler a function to call whenever the bodies collide
 * @param aux an auxiliary value to pass to the handler
 * @param freer if non-NULL, a function to call in order to free aux
 */
void create_collision(scene_t *scene, body_t *body1, body_t *body2,
	collision_handler_t handler, void *aux, free_func_t freer);

/**
 * Adds a force creator to a scene that destroys two bodies when they collide.
 * The bodies should be destroyed by calling body_remove().
 * This should be represented as an on-collision callback
 * registered with create_collision().
 *
 * @param scene the scene containing the bodies
 * @param body1 the first body
 * @param body2 the second body
 */
void create_destructive_collision(scene_t *scene, body_t *body1, body_t *body2);

/**
 * Adds a force creator to a scene that applies impulses
 * to resolve collisions between two bodies in the scene.
 * This should be represented as an on-collision callback
 * registered with create_collision().
 *
 * You may remember from project01 that you should avoid applying impulses
 * multiple times while the bodies are still colliding.
 * You should also have a special case that allows either body1 or body2
 * to have mass INFINITY, as this is useful for simulating walls.
 *
 * @param scene the scene containing the bodies
 * @param elasticity the "coefficient of restitution" of the collision;
 * 0 is a perfectly inelastic collision and 1 is a perfectly elastic collision
 * @param body1 the first body
 * @param body2 the second body
 */
void create_physics_collision(
	scene_t *scene, double elasticity, body_t *body1, body_t *body2);

/**
 * Creates a collision between a car and a wall. These collisions are highly specialized
 * to their application, and differ from standard physics collisions in numerous ways:
 *
 * 1. Uses specified collision axes from collision_ref_t info to guarantee that cars bounce
 * off walls in a controlled and repeatable fashion.
 * 2. Impulses to apply are calculated specifically for a car and (infinite mass) wall;
 * collision is resolved manually along normal and tangential directions. The wall normal
 * direction respects the coefficient of restitution, while the wall tangential direction is
 * tuned with a friction factor to prevent the "wall-riding" phenomenon.
 * 3. Car centroid is also manually adjusted away from the site of the collision, rather than
 * relying on the is_collided flag to prevent double-counting impulses in adjacent time-steps.
 * This was found to be the best method to prevent cars glitching through walls.
 * 4. Collisions are not computed for objects that are far apart.
 * 5. All car engine, braking, drag, and friction forces are disabled through the collision flag
 * during a collision timestep, to prevent cars from being forced through walls.
 *
 * @param scene the scene containing the bodies
 * @param elasticity the coefficient of restitution
 * @param collision_friction the friction factor for tangential friction during collision
 * @param body1 the car body
 * @param body2 the wall body
 */
void create_car_wall_collision(
	scene_t *scene, double elasticity, double collision_friction, body_t *body1, body_t *body2);

/**
 * Creates a collision between a car and grass. The purpose of this collision is
 * merely to set a flag in collision_ref_t which allows the car physics computation
 * to increase the drag coefficient in response, in order to slow down cars over grass.
 * This discourages poor driving habits (cutting the track, exceeding track limits), and
 * also aids in controllability of cars when entering high-speed corners.
 * 
 * @param scene the scene containing the bodies
 * @param body1 the car body
 * @param body2 the grass body
 */
void create_car_grass_collision(scene_t *scene, body_t *body1, body_t *body2);

/**
 * Creates a collision between two cars. This collision is implemented as a standard
 * physics collision, but is presented in its own function to aid possible future expansion.
 * 
 * @param scene the scene containing the bodies
 * @param elasticity the coefficient of restitution
 * @param body1 the first car body
 * @param body2 the second car body
 */
void create_car_car_collision(scene_t *scene, double elasticity, body_t *body1, body_t *body2);

/**
 * Creates a collision between a car and the beginning portion of a finish line, allowing for
 * lap timings to be tracked, as well as to determine the direction that the car crosses the line with.
 * Updates the laps_t object of the client with relevant lap count data. Uses the did_collide flag to
 * prevent double-counting lap-count increases across adjacent timesteps.
 * 
 * @param scene the scene containing the bodies
 * @param client the client owning the bodies
 * @param body1 the car body
 * @param body2 the finish line begin body
 */
void create_car_finish_begin_collision(scene_t *scene, client_t *client, body_t *body1, body_t *body2);

/**
 * Creates a collision between a car and the end portion of a finish line, allowing for
 * lap timings to be tracked, as well as to determine the direction that the car crosses the line with.
 * Updates the laps_t object of the client with relevant lap count data. Uses the did_collide flag to
 * prevent double-counting lap-count increases across adjacent timesteps.
 * 
 * @param scene the scene containing the bodies
 * @param client the client owning the bodies
 * @param body1 the car body
 * @param body2 the finish line end body
 */
void create_car_finish_end_collision(scene_t *scene, client_t *client, body_t *body1, body_t *body2);

#endif // #ifndef __FORCES_H__