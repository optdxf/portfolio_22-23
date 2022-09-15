#include "car.h"
#include "body.h"
#include "color.h"
#include "forces.h"
#include "list.h"
#include "map.h"
#include "math.h"
#include "math_util.h"
#include "shapes.h"
#include <stdio.h>
#include <stdlib.h>

/* CAR CONSTANTS */

const double CAR_MASS[] = { 20, 20 };
const double CAR_MOI[] = { 100, 100 };
const double CAR_LENGTH[] = { 43, 43 };
const double CAR_WHEELBASE[] = { 35, 35 };
const double CAR_WIDTH[] = { 20, 20 };
const char *CAR_IMAGES[] = { "assets/cars/audi.png", "assets/cars/black_viper.png" };
const rgba_color_t CAR_COLOR[] = { (rgba_color_t){}, (rgba_color_t){} };
const double CAR_ELASTICITY_WALL[] = { 0.05, 0.05 };
const double CAR_ELASTICITY_CAR[] = { 0.5, 0.5 };
const double CAR_COLLISION_FRICTION[] = {0.25, 0.25};

const double POWER_ON[] = { 5050, 5050 };
const double POWER_REVERSE[] = { -1500, -1500 };
const double POWER_OFF[] = { 0, 0 };
const double BRAKE_ON[] = { 3000, 3750 };
const double BRAKE_OFF[] = { 375, 375 };
const double GAMMA_NOMINAL[] = { 0.05, 0.05 };
const double GAMMA_GRASS[] = { 0.2, 0.2 };
const double MU_GRIP[] = { 20, 20 };
const double MU_SLIP[] = { 7.5, 7.5 };
const double MAX_STEER_OMEGA_NOMINAL[] = { 1.5, 1.25 };
const double MAX_STEER_OMEGA_BRAKE[] = { 2.75, 2.25 };

const rgba_color_t TRACKS_COLOR[] = { (rgba_color_t){ 0, 0, 0, 1 }, (rgba_color_t){ 0, 0, 0, 1 } };
const double TRACKS_OPACITY_HIGH[] = { 0.2, 0.2 };
const double THRESHOLD_ANGLE_LOW[] = { 0.2, 0.2 };
const double THRESHOLD_ANGLE_HIGH[] = { 1.0, 1.0 };
const double TRACK_WIDTH_MIN[] = { 0.5, 0.5 };
const double TRACK_WIDTH_MAX[] = { 4, 4 };
const double TRACK_LENGTH_SCALE[] = { 0.05, 0.05 };
const double TRACK_LASTING_TIME[] = { 5, 5 };
const double WHEELSPIN_SPEED[] = { 200, 200 };

struct car {
	car_type_t car_type;
	body_t *body;
	double wheelbase;
	double width;
	double length;
	double elasticity_wall;
	double elasticity_car;
	double collision_friction;

	double power_on;
	double power_reverse;
	double power_off;
	double power;

	double brake_on;
	double brake_off;
	double brake;

	double gamma;

	double mu_grip;
	double mu_slip;
	double mu;

	double max_steer_omega;
	double max_steer_omega_nominal;
	double max_steer_omega_brake;

	rgba_color_t tracks_color;
	double tracks_opacity_high;
	double threshold_angle_low;
	double threshold_angle_high;
	double track_width_min;
	double track_width_max;
	double track_length_scale;
	double track_lasting_time;
	double wheelspin_speed;

	bool colliding;
	double map_scale;
};

car_t *car_init(car_type_t car_type, double map_scale, double pos_x, double pos_y) {
	car_t *car = malloc(sizeof(*car));
	car->map_scale = map_scale;
	car->car_type = car_type;
	car->wheelbase = car->map_scale * CAR_WHEELBASE[car_type];
	car->width = car->map_scale * CAR_WIDTH[car_type];
	car->length = car->map_scale * CAR_LENGTH[car_type];
	car->elasticity_wall = CAR_ELASTICITY_WALL[car_type];
	car->elasticity_car = CAR_ELASTICITY_CAR[car_type];
	car->collision_friction = CAR_COLLISION_FRICTION[car_type];

	car->power_on = car->map_scale * POWER_ON[car_type];
	car->power_reverse = car->map_scale * POWER_REVERSE[car_type];
	car->power_off = car->map_scale * POWER_OFF[car_type];
	car->power = car->map_scale * POWER_OFF[car_type];

	car->brake_on = car->map_scale * BRAKE_ON[car_type];
	car->brake_off = car->map_scale * BRAKE_OFF[car_type];
	car->brake = car->map_scale * BRAKE_OFF[car_type];

	car->gamma = GAMMA_NOMINAL[car_type];

	car->mu_slip = MU_SLIP[car_type];
	car->mu_grip = MU_GRIP[car_type];
	car->mu = MU_GRIP[car_type];

	car->max_steer_omega_nominal = MAX_STEER_OMEGA_NOMINAL[car_type];
	car->max_steer_omega_brake = MAX_STEER_OMEGA_BRAKE[car_type];
	car->max_steer_omega = MAX_STEER_OMEGA_NOMINAL[car_type];

	car->tracks_color = TRACKS_COLOR[car_type];
	car->tracks_opacity_high = TRACKS_OPACITY_HIGH[car_type];
	car->threshold_angle_low = THRESHOLD_ANGLE_LOW[car_type];
	car->threshold_angle_high = THRESHOLD_ANGLE_HIGH[car_type];
	car->track_width_min = car->map_scale * TRACK_WIDTH_MIN[car_type];
	car->track_width_max = car->map_scale * TRACK_WIDTH_MAX[car_type];
	car->track_length_scale = car->map_scale * TRACK_LENGTH_SCALE[car_type];
	car->track_lasting_time = TRACK_LASTING_TIME[car_type];
	car->wheelspin_speed = car->map_scale * WHEELSPIN_SPEED[car_type];

	car->colliding = false;

	// add_car code
	list_t *shape = make_rectangle(
		(vector_t){ pos_x, pos_y }, car->width, car->length);
	collision_ref_t *info = malloc(sizeof(*info));
	*info = (collision_ref_t){ sqrt(pow((car->length / 2), 2) + pow((car->width / 2), 2)),
		false, false, VEC_ZERO, CAR };
	body_t *body = body_init_with_info(
		shape, car->map_scale * CAR_MASS[car_type], CAR_COLOR[car_type], (void *)info, free);
	body_set_omega_center(body, (vector_t){ -car->wheelbase / 2, 0 });
	body_set_img(body, CAR_IMAGES[car_type]);

	list_t *car_shape = body_get_shape(body);
	polygon_rotate(car_shape, -M_PI / 2, body_get_centroid(body));
	body_update_shape(body, car_shape);

	list_t *car_bounding_rect =
		make_rectangle(body_get_centroid(body), car->width, car->length);
	polygon_rotate(car_bounding_rect, -M_PI / 2, polygon_centroid(car_bounding_rect));
	body_set_bounding_rect(body, car_bounding_rect);
	car->body = body;

	body_set_rotation_rel(car->body, M_PI);
	body_set_centroid_rel(car->body, (vector_t){ car->length, 0 } );

	return car;
}

void car_set_colliding(car_t *car, bool colliding) { car->colliding = colliding; }

double car_slip_angle(body_t *body) {
	double angle = vec_angle_between(body_get_velocity(body), body_get_orientation(body));
	if (angle > M_PI) {
		angle = 2 * M_PI - angle;
	}
	if (angle > M_PI / 2) {
		angle = M_PI - angle;
	}
	return angle; // between 0 and pi/2
}

void car_compute_physics(car_t *car, input_data_t *input) {
	if (input->space_held) {
		car->power = car->map_scale * car->power_off;
		car->brake = car->map_scale * car->brake_on;
		car->mu = car->map_scale * car->mu_grip;
		car->max_steer_omega = car->max_steer_omega_brake;
	} else {
		car->max_steer_omega = car->max_steer_omega_nominal;
		if (input->up_held && !input->down_held) {
			car->power = car->map_scale * car->power_on;
			car->brake = car->map_scale * car->brake_off;
			car->mu = car->map_scale * car->mu_slip;
		} else if (input->down_held && !input->up_held) {
			car->power = car->map_scale * car->power_reverse;
			car->brake = car->map_scale * car->brake_off;
			car->mu = car->map_scale * car->mu_slip;
		} else {
			car->power = car->map_scale * car->power_off;
			car->brake = car->map_scale * car->brake_off;
			car->mu = car->map_scale * car->mu_grip;
		}
	}
	if ((input->left_held && input->right_held) ||
		(!input->left_held && !input->right_held)) {
		body_set_omega(car->body, 0);
	} else {
		if (!((collision_ref_t *)body_get_info(car->body))->collision_flag) {
			double velocity_omega = 0;
			velocity_omega = body_get_speed(car->body) / car->wheelbase;
			if (car->max_steer_omega < velocity_omega) {
				velocity_omega = car->max_steer_omega;
			}
			if (vec_dot(body_get_velocity(car->body), body_get_orientation(car->body)) >=
				0) {
				if (input->left_held) {
					body_set_omega(car->body, velocity_omega);
				} else {
					body_set_omega(car->body, -velocity_omega);
				}
			} else {
				if (input->left_held) {
					body_set_omega(car->body, -velocity_omega);
				} else {
					body_set_omega(car->body, velocity_omega);
				}
			}
		}
	}

	if (((collision_ref_t *)body_get_info(car->body))->touch_grass) {
		car->gamma = GAMMA_GRASS[car->car_type];
	} else {
		car->gamma = GAMMA_NOMINAL[car->car_type];
	}
}

float opacity_func_speed(car_t *car, double speed) {
	if (speed > car->wheelspin_speed) {
		return 0;
	}
	return (1 - speed / car->wheelspin_speed) * car->tracks_opacity_high;
}

float opacity_func_slip_angle(car_t *car, double slip_angle) {
	return slip_angle / (M_PI / 2) * car->tracks_opacity_high;
}

list_t *car_make_tracks(car_t *car, input_data_t *input) {
	list_t *objects = list_init(0, NULL);

	vector_t center = body_get_centroid(car->body);
	vector_t o = body_get_orientation(car->body);
	vector_t v = body_get_velocity(car->body);
	double s = body_get_speed(car->body);
	double slip_angle = car_slip_angle(car->body);
	bool wheelspin = ((input->up_held && !input->down_held) || input->space_held) &&
					 s < car->wheelspin_speed;
	if (slip_angle < car->threshold_angle_low && !wheelspin) {
		return objects;
	}

	double track_width = (M_PI / 2 - slip_angle) / (M_PI / 2) *
							 (car->track_width_max - car->track_width_min) +
						 car->track_width_min;

	vector_t front_translation = vec_multiply(car->wheelbase / 2, o);
	vector_t rear_translation = vec_negate(vec_multiply(car->wheelbase / 2, o));
	vector_t right_translation =
		vec_multiply(car->width / 2 - track_width / 2, vec_rotate(o, M_PI / 2));
	vector_t left_translation =
		vec_multiply(car->width / 2 - track_width / 2, vec_rotate(o, -M_PI / 2));

	double track_length = s * car->track_length_scale;
	vector_t left_rear_pos = vec_add(center, vec_add(left_translation, rear_translation));
	vector_t right_rear_pos =
		vec_add(center, vec_add(right_translation, rear_translation));
	vector_t left_front_pos =
		vec_add(center, vec_add(left_translation, front_translation));
	vector_t right_front_pos =
		vec_add(center, vec_add(right_translation, front_translation));

	list_t *left_rear_rect =
		make_rectangle(vec_subtract(left_rear_pos, (vector_t){ 0, -track_length / 2 }),
			track_width, track_length);
	list_t *right_rear_rect =
		make_rectangle(vec_subtract(right_rear_pos, (vector_t){ 0, -track_length / 2 }),
			track_width, track_length);
	list_t *left_front_rect =
		make_rectangle(vec_subtract(left_front_pos, (vector_t){ 0, -track_length / 2 }),
			track_width, track_length);
	list_t *right_front_rect =
		make_rectangle(vec_subtract(right_front_pos, (vector_t){ 0, -track_length / 2 }),
			track_width, track_length);

	double rotate_angle = vec_angle_between(v, (vector_t){ 1, 0 });
	polygon_rotate(left_rear_rect, M_PI / 2 + rotate_angle, left_rear_pos);
	polygon_rotate(right_rear_rect, M_PI / 2 + rotate_angle, right_rear_pos);
	polygon_rotate(left_front_rect, M_PI / 2 + rotate_angle, left_front_pos);
	polygon_rotate(right_front_rect, M_PI / 2 + rotate_angle, right_front_pos);

	float opacity = opacity_func_slip_angle(car, slip_angle);
	if (wheelspin && opacity_func_speed(car, s) > opacity) {
		opacity = opacity_func_speed(car, s);
	}

	collision_ref_t *left_info = malloc(sizeof(*left_info));
	*left_info = (collision_ref_t){ 0, false, false, VEC_ZERO, TRACK };
	body_t *left_rear = body_init_with_info(
		left_rear_rect, INFINITY, color_set_opacity(car->tracks_color, opacity), (void *)left_info, free);
	list_add(objects, left_rear);

	collision_ref_t *right_info = malloc(sizeof(*right_info));
	*right_info = (collision_ref_t){ 0, false, false, VEC_ZERO, TRACK };
	body_t *right_rear = body_init_with_info(
		right_rear_rect, INFINITY, color_set_opacity(car->tracks_color, opacity), (void *)right_info, free);
	list_add(objects, right_rear);

	// TRACKS FOR FRONT WHEELS

	// if (vec_dot(v, o) < 0 && slip_angle > THRESHOLD_ANGLE_LOW) {
	// 	body_t *left_front = body_init(
	// 		left_front_rect, INFINITY, color_set_opacity(TRACKS_COLOR, opacity));
	// 	scene_add_body(state->scene, left_front);

	// 	body_t *right_front = body_init(
	// 		right_front_rect, INFINITY, color_set_opacity(TRACKS_COLOR, opacity));;
	// 	scene_add_body(state->scene, right_front);
	// }

	return objects;
}

double *car_get_power_ptr(car_t *car) { return &car->power; }

double *car_get_brake_ptr(car_t *car) { return &car->brake; }

double *car_get_mu_ptr(car_t *car) { return &car->mu; }

double *car_get_gamma_ptr(car_t *car) { return &car->gamma; }

double car_get_track_lasting_time(car_t *car) { return car->track_lasting_time; }

double car_get_elasticity_wall(car_t *car) { return car->elasticity_wall; }

double car_get_elasticity_car(car_t *car) { return car->elasticity_car; }

double car_get_collision_friction(car_t *car) {return car->collision_friction; }

body_t *car_get_body(car_t *car) { return car->body; }

void car_set_position(car_t *car, vector_t position) {
	body_set_centroid(car_get_body(car), position);
}

car_type_t car_get_type(car_t *car) {
	return car->car_type;
}