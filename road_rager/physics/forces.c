#include "forces.h"
#include "collision.h"
#include "client.h"
#include "server.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//-- CONSTANTS

// how far away two bodies must be before newtonian forces come into effect
const double MIN_NEWTONIAN_DISTANCE = 100;
const double G_ACCEL = 9.81; // gravitational acceleration
const double EPSILON = 1;
const double CAR_CAR_EPSILON = 1;
const double VEL_BOUNCE_SCALE = 1;
const double RADIUS_IGNORE_SCALE = 2;
const double TAN_IGNORE_ANGLE = 0.2;

//-- INTERNAL FORCE CREATORS (INTERNAL API)

typedef struct {
	double *mu;
} friction_data_t;

typedef struct {
	double max;
	clock_t start;
} timer_data_t;

void newtonian_force_creator(list_t *bodies, double *data) {
	body_t *body1 = list_get(bodies, 0);
	body_t *body2 = list_get(bodies, 1);
	double radius = body_get_distance(body1, body2);
	if (radius < MIN_NEWTONIAN_DISTANCE) {
		// we return right away instead of choosing a min radius b/c this behavior
		// yields the best results
		return;
	}
	double force_value =
		*data * body_get_mass(body1) * body_get_mass(body2) / (radius * radius);
	vector_t dir1 =
		vec_unit_vec(vec_subtract(body_get_centroid(body2), body_get_centroid(body1)));
	vector_t dir2 = vec_negate(dir1);
	body_add_force(body1, vec_multiply(force_value, dir1));
	body_add_force(body2, vec_multiply(force_value, dir2));
}

void spring_force_creator(list_t *bodies, double *data) {
	body_t *body1 = list_get(bodies, 0);
	body_t *body2 = list_get(bodies, 1);
	double force_value = *data * body_get_distance(body1, body2);
	vector_t dir1 =
		vec_unit_vec(vec_subtract(body_get_centroid(body2), body_get_centroid(body1)));
	vector_t dir2 = vec_negate(dir1);
	body_add_force(body1, vec_multiply(force_value, dir1));
	body_add_force(body2, vec_multiply(force_value, dir2));
}

void drag_force_creator(list_t *bodies, double *data) {
	body_add_force((body_t *)list_get(bodies, 0),
		vec_multiply(-(*data), body_get_velocity((body_t *)list_get(bodies, 0))));
}

void car_drag_force_creator(list_t *bodies, double *data) {
	body_t *body = (body_t *)list_get(bodies, 0);
	if (!((collision_ref_t *)body_get_info(body))->collision_flag) {
		body_add_force(
			body, vec_multiply(-(*data) * body_get_speed(body), body_get_velocity(body)));
	}
}

void dynamic_engine_force_creator(list_t *bodies, double *data) {
	body_t *body = (body_t *)list_get(bodies, 0);
	if (!((collision_ref_t *)body_get_info(body))->collision_flag) {
		body_add_force(body, vec_multiply(*data, body_get_orientation(body)));
	}
}

void dynamic_braking_force_creator(list_t *bodies, double *data) {
	body_t *body = (body_t *)list_get(bodies, 0);
	if (!((collision_ref_t *)body_get_info(body))->collision_flag) {
		if (vec_isclose(body_get_velocity(body), VEC_ZERO)) {
			return;
		}
		vector_t brake_force = vec_multiply(*data, body_get_orientation(body));
		if (vec_dot(body_get_velocity(body), body_get_orientation(body)) > 0) {
			brake_force = vec_negate(brake_force);
		}
		body_add_force(body, brake_force);
	}
}

void friction_force_creator(list_t *bodies, friction_data_t *data) {
	body_t *body = (body_t *)list_get(bodies, 0);

	vector_t v = body_get_velocity(body);
	if (vec_isclose(v, VEC_ZERO)) {
		// no friction for stationary objects
		return;
	}

	vector_t o = body_get_orientation(body);
	double mag = *(data->mu) * body_get_mass(body) * G_ACCEL;

	// friction acts orthogonally to the objects orientation, opposing relative slip
	vector_t dir = vec_rotate(o, M_PI / 2);
	if (vec_dot(v, dir) > 0) {
		dir = vec_negate(dir);
	}

	body_add_force(body, vec_multiply(mag, dir));
}

void destructive_collision_creator(list_t *bodies, void *empty) {
	body_t *body1 = (body_t *)list_get(bodies, 0);
	body_t *body2 = (body_t *)list_get(bodies, 1);
	list_t *shape1 = body_get_shape_pointer(body1);
	list_t *shape2 = body_get_shape_pointer(body2);

	if (find_collision(shape1, shape2).collided) {
		body_remove(body1);
		body_remove(body2);
	}
}

void timed_remover_creator(list_t *bodies, timer_data_t *time) {
	if ((double)(clock() - time->start) / CLOCKS_PER_SEC >= time->max) {
		body_remove(list_get(bodies, 0));
	}
}

//--- PUBLIC API

void create_newtonian_gravity(scene_t *scene, double G, body_t *body1, body_t *body2) {
	double *data = malloc(sizeof(double));
	*data = G;
	list_t *bodies = list_init(2, NULL);
	list_add(bodies, body1);
	list_add(bodies, body2);
	scene_add_bodies_force_creator(
		scene, (force_creator_t)newtonian_force_creator, data, bodies, free);
}

void create_spring(scene_t *scene, double k, body_t *body1, body_t *body2) {
	double *data = malloc(sizeof(double));
	*data = k;
	list_t *bodies = list_init(2, NULL);
	list_add(bodies, body1);
	list_add(bodies, body2);
	scene_add_bodies_force_creator(
		scene, (force_creator_t)spring_force_creator, data, bodies, free);
}

void create_drag(scene_t *scene, double gamma, body_t *body) {
	double *data = malloc(sizeof(double));
	*data = gamma;
	list_t *bodies = list_init(1, NULL);
	list_add(bodies, body);
	scene_add_bodies_force_creator(
		scene, (force_creator_t)drag_force_creator, data, bodies, free);
}

void create_car_drag(scene_t *scene, double *gamma, body_t *body) {
	list_t *bodies = list_init(1, NULL);
	list_add(bodies, body);
	scene_add_bodies_force_creator(
		scene, (force_creator_t)car_drag_force_creator, gamma, bodies, NULL);
}

void create_destructive_collision(scene_t *scene, body_t *body1, body_t *body2) {
	list_t *bodies = list_init(2, NULL);
	list_add(bodies, body1);
	list_add(bodies, body2);
	scene_add_bodies_force_creator(
		scene, (force_creator_t)destructive_collision_creator, NULL, bodies, NULL);
}

void create_dynamic_engine(scene_t *scene, double *power, body_t *body) {
	list_t *bodies = list_init(1, NULL);
	list_add(bodies, body);
	scene_add_bodies_force_creator(
		scene, (force_creator_t)dynamic_engine_force_creator, power, bodies, NULL);
}

void create_dynamic_braking(scene_t *scene, double *power, body_t *body) {
	list_t *bodies = list_init(1, NULL);
	list_add(bodies, body);
	scene_add_bodies_force_creator(
		scene, (force_creator_t)dynamic_braking_force_creator, power, bodies, NULL);
}

void create_friction(scene_t *scene, double *mu, body_t *body) {
	friction_data_t *data = malloc(sizeof(*data));
	data->mu = mu;
	list_t *bodies = list_init(1, NULL);
	list_add(bodies, body);
	scene_add_bodies_force_creator(
		scene, (force_creator_t)friction_force_creator, data, bodies, NULL);
}

void create_timed_remover(scene_t *scene, double seconds, body_t *body) {
	timer_data_t *data = malloc(sizeof(*data));
	data->max = seconds;
	data->start = clock();
	list_t *bodies = list_init(1, NULL);
	list_add(bodies, body);
	scene_add_bodies_force_creator(
		scene, (force_creator_t)timed_remover_creator, data, bodies, free);
}

typedef struct {
	bool did_collide;
	double elasticity;
} physics_collision_data_t;

void physics_collision_creator(list_t *bodies, physics_collision_data_t *data) {
	body_t *body1 = list_get(bodies, 0);
	body_t *body2 = list_get(bodies, 1);
	list_t *shape1 = body_get_shape_pointer(body1);
	list_t *shape2 = body_get_shape_pointer(body2);
	collision_info_t collision = find_collision(shape1, shape2);

	if (!data->did_collide && collision.collided) {
		data->did_collide = true;

		vector_t axis = collision.axis;
		double u1 = vec_dot(body_get_velocity(body1), axis);
		double u2 = vec_dot(body_get_velocity(body2), axis);
		double m1 = body_get_mass(body1);
		double m2 = body_get_mass(body2);

		double mass_factor;
		if (m1 != INFINITY && m2 != INFINITY) {
			mass_factor = (m1 * m2) / (m1 + m2);
		} else if (m1 == INFINITY) {
			mass_factor = m2;
		} else {
			mass_factor = m1;
		}
		vector_t impulse1 =
			vec_multiply(mass_factor * (1 + data->elasticity) * (u2 - u1), axis);
		vector_t impulse2 = vec_negate(impulse1);

		body_add_impulse(body1, impulse1);
		body_add_impulse(body2, impulse2);
	} else if (data->did_collide && !collision.collided) {
		data->did_collide = false;
	}
}

void create_physics_collision(
	scene_t *scene, double elasticity, body_t *body1, body_t *body2) {
	list_t *bodies = list_init(2, NULL);
	list_add(bodies, body1);
	list_add(bodies, body2);
	physics_collision_data_t *data = malloc(sizeof(*data));
	data->elasticity = elasticity;
	data->did_collide = false;
	scene_add_bodies_force_creator(
		scene, (force_creator_t)physics_collision_creator, data, bodies, free);
}

//-- CAR COLLISIONS START HERE

typedef struct {
	double elasticity;
	double collision_friction;
	double *delta;
} car_wall_collision_data_t;

void car_wall_collision_creator(list_t *bodies, car_wall_collision_data_t *data) {
	body_t *body1 = list_get(bodies, 0);
	body_t *body2 = list_get(bodies, 1);
	double radius1 = ((collision_ref_t *)body_get_info(body1))->bounding_circle_radius;
	double radius2 = ((collision_ref_t *)body_get_info(body2))->bounding_circle_radius;
	double distance = body_get_distance(body1, body2);

	// Assume first flag is that of car
	bool flag1 = ((collision_ref_t *)body_get_info(body1))->collision_flag;

	// Check whether distances are far enough away to avoid checking collisions
	if (distance <= RADIUS_IGNORE_SCALE * (radius1 + radius2)) {
		list_t *shape1 = body_get_shape_pointer(body1);
		list_t *shape2 = body_get_shape_pointer(body2);
		collision_info_t collision = find_collision(shape1, shape2);

		if (!flag1 && collision.collided) {
			// printf("collision \n");
			// set flags
			((collision_ref_t *)body_get_info(body1))->collision_flag = true;

			vector_t axis = ((collision_ref_t *)body_get_info(body2))->orientation_vector;
			double u1 = vec_dot(body_get_velocity(body1), axis);
			double m1 = body_get_mass(body1);

			vector_t impulse = vec_multiply(-m1 * (1 + data->elasticity) * u1, axis);
			body_add_impulse(body1, impulse);

			vector_t delta =
				vec_multiply(-VEL_BOUNCE_SCALE * *data->delta, body_get_velocity(body1));
			if (vec_dot(body_get_velocity(body1), axis) > 0) {
				delta = vec_add(delta, vec_multiply(-EPSILON, vec_unit_vec(axis)));
			} else {
				delta = vec_add(delta, vec_multiply(EPSILON, vec_unit_vec(axis)));
			}
			body_set_centroid_rel(body1, delta);

			double tan_angle = vec_angle_between(axis, body_get_velocity(body1));
			if (tan_angle > M_PI) { tan_angle = 2 * M_PI - tan_angle; }

			if (tan_angle > TAN_IGNORE_ANGLE){
				vector_t axis_tan = vec_rotate(axis, M_PI / 2);
				double u1_tan = vec_dot(body_get_velocity(body1), axis_tan);
				// friction factor
				vector_t impulse_tan =
					vec_multiply(-m1 * (data->collision_friction) * u1_tan, axis_tan);
				body_add_impulse(body1, impulse_tan);
			}
		}
	}
}

void create_car_wall_collision(
	scene_t *scene, double elasticity, double collision_friction, body_t *body1, body_t *body2) {
	list_t *bodies = list_init(2, NULL);
	list_add(bodies, body1);
	list_add(bodies, body2);
	car_wall_collision_data_t *data = malloc(sizeof(*data));
	data->elasticity = elasticity;
	data->collision_friction = collision_friction;
	data->delta = scene_get_delta_ptr(scene);
	scene_add_bodies_force_creator(
		scene, (force_creator_t)car_wall_collision_creator, data, bodies, free);
}

void car_grass_collision_creator(list_t *bodies, void *data) {
	body_t *body1 = list_get(bodies, 0);
	body_t *body2 = list_get(bodies, 1);

	list_t *shape1 = body_get_shape_pointer(body1);
	list_t *shape2 = body_get_shape_pointer(body2);
	collision_info_t collision = find_collision(shape1, shape2);

	if (collision.collided) {
		((collision_ref_t *)body_get_info(body1))->touch_grass = true;
	}
}

void create_car_grass_collision(scene_t *scene, body_t *body1, body_t *body2) {
	list_t *bodies = list_init(2, NULL);
	list_add(bodies, body1);
	list_add(bodies, body2);
	scene_add_bodies_force_creator(
		scene, (force_creator_t)car_grass_collision_creator, NULL, bodies, free);
}

typedef struct {
	double elasticity;
	double *delta;
} car_car_collision_data_t;

void car_car_collision_creator(list_t *bodies, car_car_collision_data_t *data) {
	body_t *body1 = list_get(bodies, 0);
	body_t *body2 = list_get(bodies, 1);
	double radius1 = ((collision_ref_t *)body_get_info(body1))->bounding_circle_radius;
	double radius2 = ((collision_ref_t *)body_get_info(body2))->bounding_circle_radius;
	double distance = body_get_distance(body1, body2);

	// Assume first flag is that of car
	bool flag1 = ((collision_ref_t *)body_get_info(body1))->collision_flag;
	bool flag2 = ((collision_ref_t *)body_get_info(body2))->collision_flag;

	if (flag1 && flag2) {
		// both cars are already colliding with something else (possibly each other)
		return;
	}

	list_t *shape1 = body_get_shape_pointer(body1);
	list_t *shape2 = body_get_shape_pointer(body2);
	collision_info_t collision = find_collision(shape1, shape2);

	if (collision.collided) {
		// set flags
		((collision_ref_t *)body_get_info(body1))->collision_flag = true;
		((collision_ref_t *)body_get_info(body2))->collision_flag = true;

		vector_t axis = collision.axis;
		double u1 = vec_dot(body_get_velocity(body1), axis);
		double u2 = vec_dot(body_get_velocity(body2), axis);
		double m1 = body_get_mass(body1);
		double m2 = body_get_mass(body2);

		if (!flag1 && !flag2) {
			double mass_factor = (m1 * m2) / (m1 + m2);

			vector_t impulse1 =
			vec_multiply(mass_factor * (1 + data->elasticity) * (u2 - u1), axis);
			vector_t impulse2 = vec_negate(impulse1);

			body_add_impulse(body1, impulse1);
			body_add_impulse(body2, impulse2);
		} else if (flag1) {
			vector_t impulse = vec_multiply(-m2 * (1 + data->elasticity) * u2, axis);
			body_add_impulse(body2, impulse);
		} else {
			vector_t impulse = vec_multiply(-m1 * (1 + data->elasticity) * u1, axis);
			body_add_impulse(body1, impulse);
		}

		vector_t pos_difference = vec_subtract(body_get_centroid(body1), body_get_centroid(body2));
		bool body_ordering = true;
		if (vec_dot(pos_difference, axis) < 0) { body_ordering = false; } // true if body1 is in front of body2

		// translate car 1 if its not already colliding
		if (!flag1) {
			vector_t delta = VEC_ZERO;
			if (body_ordering) {
				delta = vec_multiply(CAR_CAR_EPSILON, vec_unit_vec(axis));
			} else {
				delta = vec_multiply(-CAR_CAR_EPSILON, vec_unit_vec(axis));
			}
			body_set_centroid_rel(body1, delta);
		}

		// translate car 2 if its not already colliding
		if (!flag2) {
			vector_t delta = VEC_ZERO;
			if (!body_ordering) {
				delta = vec_multiply(CAR_CAR_EPSILON, vec_unit_vec(axis));
			} else {
				delta = vec_multiply(-CAR_CAR_EPSILON, vec_unit_vec(axis));
			}
			body_set_centroid_rel(body2, delta);
		}
	}
}

void create_car_car_collision(scene_t *scene, double elasticity, body_t *body1, body_t *body2) {
	list_t *bodies = list_init(2, NULL);
	list_add(bodies, body1);
	list_add(bodies, body2);
	car_car_collision_data_t *data = malloc(sizeof(*data));
	data->elasticity = elasticity;
	data->delta = scene_get_delta_ptr(scene);
	scene_add_bodies_force_creator(
		scene, (force_creator_t)car_car_collision_creator, data, bodies, free);
}

typedef struct {
	client_t *client;
	bool did_collide;
} car_finish_collision_data_t;

void car_finish_begin_collision_creator(list_t *bodies, car_finish_collision_data_t *data) {
	body_t *body1 = list_get(bodies, 0);
	body_t *body2 = list_get(bodies, 1);

	//printf("collision happening??\n");

	list_t *shape1 = body_get_shape_pointer(body1);
	list_t *shape2 = body_get_shape_pointer(body2);
	collision_info_t collision = find_collision(shape1, shape2);

	if (!data->did_collide && collision.collided) {
		data->did_collide = true;

		client_t *client = data->client;
		laps_t *client_laps = client_get_laps(client);
			
		if (client_laps->finish_state == 2) {
			// passing through finish line begin
			client_laps->finish_state = 0;
		} else if (client_laps->finish_state == 1) {
			// going backwards
			client_laps->lap_count--;
			client_laps->finish_state = 2;
		}
	} else if (data->did_collide && !collision.collided) {
		data->did_collide = false;
	}
}

void create_car_finish_begin_collision(scene_t *scene, client_t *client, body_t *body1, body_t *body2) {
	list_t *bodies = list_init(2, NULL);
	list_add(bodies, body1);
	list_add(bodies, body2);

	car_finish_collision_data_t *data = malloc(sizeof(*data));
	data->client = client;
	data->did_collide = false;

	scene_add_bodies_force_creator(
		scene, (force_creator_t)car_finish_begin_collision_creator, data, bodies, free);
}

void car_finish_end_collision_creator(list_t *bodies, car_finish_collision_data_t *data) {
	body_t *body1 = list_get(bodies, 0);
	body_t *body2 = list_get(bodies, 1);

	list_t *shape1 = body_get_shape_pointer(body1);
	list_t *shape2 = body_get_shape_pointer(body2);
	collision_info_t collision = find_collision(shape1, shape2);

	if (!data->did_collide && collision.collided) {
		data->did_collide = true;

		client_t *client = data->client;
		laps_t *client_laps = client_get_laps(client);
		if (client_laps->finish_state == 0) {
			client_laps->lap_count++;
			printf("client: %i | %i\n", client_get_id(client), client_laps->lap_count);
			if (client_laps->lap_count > client_laps->lap_max && client_laps->lap_count > 0) {
				//if player has hit a lap max, give bonus for singleplayer
				client_laps->time_remaining += singleplayer_extra_time(client_laps->lap_count, client_get_map(client));
				client_laps->lap_max = client_laps->lap_count;
			}
			client_laps->finish_state = 2;
		} else if (client_laps->finish_state == 2) {
			//account for car between finish lines
			client_laps->finish_state = 1;
		}
	} else if (data->did_collide && !collision.collided) {
		data->did_collide = false;
	}
}

void create_car_finish_end_collision(scene_t *scene, client_t *client, body_t *body1, body_t *body2) {
	list_t *bodies = list_init(2, NULL);
	list_add(bodies, body1);
	list_add(bodies, body2);

	car_finish_collision_data_t *data = malloc(sizeof(*data));
	data->client = client;
	data->did_collide = false;

	scene_add_bodies_force_creator(
		scene, (force_creator_t)car_finish_end_collision_creator, data, bodies, free);
}

//-- CAR COLLISIONS END HERE

typedef struct {
	collision_handler_t handler;
	void *aux;
	free_func_t freer;
	bool did_collide;
} collision_data_t;

void free_collision_data(collision_data_t *data) {
	if (data->freer) {
		data->freer(data->aux);
	}
	free(data);
}

void collision_creator(list_t *bodies, collision_data_t *data) {
	body_t *body1 = list_get(bodies, 0);
	body_t *body2 = list_get(bodies, 1);
	list_t *shape1 = body_get_shape_pointer(body1);
	list_t *shape2 = body_get_shape_pointer(body2);
	collision_info_t collision = find_collision(shape1, shape2);
	vector_t axis = collision.axis;

	if (!data->did_collide && collision.collided) {
		data->did_collide = true;
		data->handler(body1, body2, axis, data->aux);
	} else if (data->did_collide && !collision.collided) {
		data->did_collide = false;
	}
}

void create_collision(scene_t *scene, body_t *body1, body_t *body2,
	collision_handler_t handler, void *aux, free_func_t freer) {
	list_t *bodies = list_init(2, NULL);
	list_add(bodies, body1);
	list_add(bodies, body2);
	collision_data_t *data = malloc(sizeof(*data));
	data->handler = handler;
	data->aux = aux;
	data->freer = freer;
	data->did_collide = false;
	scene_add_bodies_force_creator(scene, (force_creator_t)collision_creator, data,
		bodies, (free_func_t)free_collision_data);
}