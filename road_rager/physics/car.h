#ifndef __CAR_H__
#define __CAR_H__

#include "body.h"
#include "input_data.h"
#include "list.h"

typedef struct car car_t;

/**
* Represents the different types of cars present in the game.
*/
typedef enum { CAR_AUDI, CAR_BLACK_VIPER } car_type_t;

/**
* Initializes a car_t object based on the input parameters.
* @param car_type the car's type enum
* @param map_scale scale various distance-based physics coefficients based on the map scale
* @param pos_x the x-coordinate to initialize the car at
* @param pos_y the y-coordinate to initialize the car at
*/
car_t *car_init(car_type_t car_type, double map_scale, double pos_x, double pos_y);

/**
* Computes the car physics based on keyboard input data from the client,
* in terms of instantaneous, dynamic state variables that adjust the output
* of the relevant force creators governing vehicle dynamics.
* @param input the keyboard input data object from the client
*/
void car_compute_physics(car_t *car, input_data_t *input);

/**
* Creates rectangles positioned at the coordinates of the car's wheels, used to
* draw tire skidmarks under certain traction-limited situations:
* 
* 1. When a car is accelerating or braking hard from a near-stand-still, create tracks
* to represent wheelspin or brake lock-up respectively.
* 2. When the car's slip angle exceeds a certain threshold, create tracks with width
* scaled based on the slip angle in order to simulate real drifting tracks (in
* particular, the tracks are thinner for a higher slip angle since the tire has a 
* narrower effective contact patch).
* 
* The tracks have a color opacity scaled to the slip angle and relative velocity between
* the tire surface and the trac* They also have lengths scaled to the velocityof the car to allow for limited server tick
* rates to generate smooth, unbroken lines. evenfinite * 

* * @param input the keyboard input data object from the client
* @return a list of objects representing the tracks created beneath the
* wheels of the car within a single timestep.
*/
list_t *car_make_tracks(car_t *car, input_data_t *input);

/**
* @return pointer to dynamic power engine variable
*/
double *car_get_power_ptr(car_t *car);

/**
* @return pointer to dynamic braking coefficient variable
*/
double *car_get_brake_ptr(car_t *car);

/**
* @return pointer to dynamic frictional coefficient variable
*/
double *car_get_mu_ptr(car_t *car);

/**
* @return pointer to dynamic drag coefficient variable
*/
double *car_get_gamma_ptr(car_t *car);

/**
* @return how long a car's tracks will last
*/
double car_get_track_lasting_time(car_t *car);

/**
* @return coefficient of restitution between car and wall
*/
double car_get_elasticity_wall(car_t *car);

/**
* @return coefficient of restitution between car and car
*/
double car_get_elasticity_car(car_t *car);

/**
* @return friction factor for car collisions
*/
double car_get_collision_friction(car_t *car);

/**
* @return body_t object belonging to car
*/
body_t *car_get_body(car_t *car);

/**
* @return enum representing car's type
*/
car_type_t car_get_type(car_t *car);

/**
* Allow manual positioning of the car (e.g. during session initialization).
* 
* @param position position vector to set car's centroid to
* @return set the car's centroid manually 
*/
void car_set_position(car_t *car, vector_t position);

#endif