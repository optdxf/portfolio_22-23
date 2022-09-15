#include "body.h"
#include "forces.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

const double MIN_SPEED = 20;

typedef struct body {
	list_t *shape;
	list_t *bounding_rect;
	double mass;
	double MOI;
	rgba_color_t color;
	vector_t centroid;
	vector_t velocity;
	double omega;
	vector_t omega_center;
	vector_t acceleration;
	double absolute_angle;
	vector_t force;
	double torque;
	vector_t impulse;
	double angular_impulse;
	void *info;
	free_func_t info_freer;
	bool to_remove;
	const char *img_fpath;
} body_t;

body_type_t *make_type_info(body_type_t type) {
	body_type_t *info = malloc(sizeof(*info));
	assert("could not allocate memory" && info != NULL);
	*info = type;
	return info;
}

body_t *body_init(list_t *shape, double mass, rgba_color_t color) {
	body_t *body = malloc(sizeof(body_t));
	assert("could not allocate memory" && body != NULL);
	assert("body should have mass > 0" && mass > 0);

	body->shape = shape;
	body->bounding_rect = NULL;
	body->mass = mass;
	body->MOI = 0;
	body->color = color;
	body->centroid = polygon_centroid(shape);
	body->velocity = VEC_ZERO;
	body->omega = 0;
	body->omega_center = VEC_ZERO;
	body->acceleration = VEC_ZERO;
	body->absolute_angle = 0;
	body->force = VEC_ZERO;
	body->impulse = VEC_ZERO;
	body->info = NULL;
	body->info_freer = NULL;
	body->to_remove = false;
	body->img_fpath = NULL;

	return body;
}

void body_set_bounding_rect(body_t *body, list_t *bounding_rect) {
	body->bounding_rect = bounding_rect;
}

list_t *body_get_bounding_rect(body_t *body) {
	if (body->bounding_rect) {
		list_t *list = list_init(list_size(body->bounding_rect), free);
		for (size_t idx = 0; idx < list_size(body->bounding_rect); ++idx) {
			vector_t *new = malloc(sizeof(vector_t));
			assert("could not allocate memory" && new != NULL);

			*new = *(vector_t *)list_get(body->bounding_rect, idx);
			list_add(list, new);
		}
		return list;
	} else {
		return NULL;
	}
}

bool body_has_bounding_rect(body_t *body) { return body->bounding_rect != NULL; }

void body_set_img(body_t *body, const char *fpath) { body->img_fpath = fpath; }

// bool body_has_img(body_t *body) { return body->img_fpath; }

const char *body_get_img(body_t *body) { return body->img_fpath; }

void body_update_shape(body_t *body, list_t *new_shape) {
	list_free(body->shape);
	body->shape = new_shape;
}

body_t *body_init_with_info(
	list_t *shape, double mass, rgba_color_t color, void *info, free_func_t info_freer) {
	body_t *body = body_init(shape, mass, color);
	body->info = info;
	body->info_freer = info_freer;
	return body;
}

void body_free(body_t *body) {
	list_free(body->shape);
	if (body->info_freer) {
		body->info_freer(body->info);
	}
	free(body);
}

list_t *body_get_shape_pointer(body_t *body) { return body->shape; }

list_t *body_get_shape(body_t *body) {
	list_t *list = list_init(list_size(body->shape), free);
	for (size_t idx = 0; idx < list_size(body->shape); ++idx) {
		vector_t *new = malloc(sizeof(vector_t));
		*new = *(vector_t *)list_get(body->shape, idx);
		list_add(list, new);
	}
	return list;
}

void *body_get_info(body_t *body) { return body->info; }

double body_get_mass(body_t *body) { return body->mass; }

double body_get_MOI(body_t *body) { return body->MOI; }

void body_set_MOI(body_t *body, double MOI) { body->MOI = MOI; }

vector_t body_get_centroid(body_t *body) { return body->centroid; }

vector_t body_get_velocity(body_t *body) { return body->velocity; }

double body_get_omega(body_t *body) { return body->omega; }

vector_t body_get_omega_center(body_t *body) { return body->omega_center; }

vector_t body_get_orientation(body_t *body) {
	return vec_rotate((vector_t){ 1, 0 }, body->absolute_angle);
}

double body_get_speed(body_t *body) { return vec_magnitude(body->velocity); }

rgba_color_t body_get_color(body_t *body) { return body->color; }

void body_set_centroid(body_t *body, vector_t x) {
	vector_t delta = vec_subtract(x, body->centroid);
	body_set_centroid_rel(body, delta);
}

void body_set_centroid_rel(body_t *body, vector_t delta) {
	polygon_translate(body->shape, delta);
	body->centroid = vec_add(body->centroid, delta);
	if (body_has_bounding_rect(body)) {
		polygon_translate(body->bounding_rect, delta);
	}
}

void body_set_velocity(body_t *body, vector_t v) { body->velocity = v; }

void body_set_omega(body_t *body, double omega) { body->omega = omega; }

void body_set_omega_center(body_t *body, vector_t omega_center) {
	body->omega_center = omega_center;
}

void body_set_rotation(body_t *body, double angle) {
	body_set_rotation_rel(body, angle - body->absolute_angle);
}

void body_set_rotation_rel(body_t *body, double angle) {
	vector_t about =
		vec_add(body->centroid, vec_rotate(body->omega_center, body->absolute_angle));
	polygon_rotate(body->shape, angle, about);
	vector_t old_centroid = body->centroid;
	body->centroid = vec_rotate_about(old_centroid, about, angle);
	if (body_has_bounding_rect(body)) {
		polygon_translate(
			body->bounding_rect, vec_subtract(body->centroid, old_centroid));
	}
	body->absolute_angle += angle;
}

void body_set_info(body_t *body, void *info) { body->info = info; }

double body_get_rotation(body_t *body) { return body->absolute_angle; }

void body_set_acceleration(body_t *this, vector_t acc) { this->acceleration = acc; }

vector_t body_get_acceleration(body_t *body) { return body->acceleration; }

bool body_contains(body_t *outer, body_t *inner) {
	return polygon_contain(outer->shape, inner->shape);
}

bool body_check_rel_pos(body_t *this, rel_pos direction, double value) {
	return polygon_check_rel_pos(this->shape, direction, value);
}

double body_get_distance(body_t *one, body_t *two) {
	return vec_magnitude(vec_subtract(one->centroid, two->centroid));
}

void body_add_force(body_t *body, vector_t force) {
	body->force = vec_add(body->force, force);
}

void body_add_torque(body_t *body, double torque) {
	body->torque = body->torque + torque;
	printf("body torque is now %.9f\n", body->torque);
}

void body_add_impulse(body_t *body, vector_t impulse) {
	body->impulse = vec_add(body->impulse, impulse);
}

void body_add_angular_impulse(body_t *body, double angular_impulse) {
	body->angular_impulse = body->angular_impulse + angular_impulse;
}

void body_tick(body_t *this, double delta) {
	if (this->mass != INFINITY) {
		// maintain backwards compatibility
		// multiply by 0.5 to account for velocity averaging over a timestep
		body_set_velocity(
			this, vec_add(this->velocity, vec_multiply(0.5 * delta, this->acceleration)));

		// implement forces as an impulse over the timestep
		// multiply by 0.5 to account for velocity averaging over a timestep
		body_add_impulse(this, vec_multiply(delta, this->force));
		body_set_velocity(this,
			vec_add(this->velocity, vec_multiply((0.5 / this->mass), this->impulse)));

		body_set_centroid(
			this, vec_add(this->centroid, vec_multiply(delta, this->velocity)));

		// multiply by 0.5 to account for velocity averaging over a timestep
		body_set_velocity(
			this, vec_add(this->velocity, vec_multiply(0.5 * delta, this->acceleration)));
		body_set_velocity(this,
			vec_add(this->velocity, vec_multiply((0.5 / this->mass), this->impulse)));

		if (this->MOI != 0) {
			body_add_angular_impulse(this, delta * this->torque);
			body_set_omega(this, this->omega + this->angular_impulse / this->MOI);
		}

		if (this->omega != 0) {
			body_set_rotation_rel(this, this->omega * delta);
		}

		this->force = VEC_ZERO;
		this->impulse = VEC_ZERO;
		this->torque = 0;
		this->angular_impulse = 0;

		if (((collision_ref_t *)this->info)->collision_flag) {
			//printf("setting flag to false\n");
			((collision_ref_t *)this->info)->collision_flag = false;
		}
	}
}

void body_remove(body_t *body) { body->to_remove = true; }

bool body_is_removed(body_t *body) { return body->to_remove; }
