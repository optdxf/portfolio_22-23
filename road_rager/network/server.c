#define _POSIX_C_SOURCE 199309L

#include "server.h"
#include "global_constants.h"
#include "car.h"
#include "forces.h"
#include "collision.h"
#include "list.h"
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <limits.h>

const double SINGLEPLAYER_INIT_TIME_MAP1 = 23;
const double SINGLEPLAYER_INIT_TIME_MAP2 = 63;
const size_t MULTIPLAYER_MAX_LAPS = 5;
const double COUNTDOWN_TIME = 3;

struct server {
	int server_id;
	server_state_t server_state;
	server_type_t server_type;
	struct timespec start_wait_time;
	struct timespec last_tick;
	double virtual_end_time;
	scene_t *scene;
	map_t *map;
	list_t *clients;
	bool game_start;
};

double singleplayer_extra_time(int lap_count, map_t *map) {
	
	if (map_get_type(map) == MAP_ONE && lap_count < 20) {
		return (15.0 - 0.5 * (double)lap_count);
	}
	else if (map_get_type(map) == MAP_TWO && lap_count < 25) {
		return (45.0 - 2 * (double)lap_count);
	}
	return 0;
}

scene_t *server_get_scene(server_t *server) { return server->scene; }

void server_set_scene(server_t *server, scene_t *scene) { server->scene = scene; }

server_t *server_init(server_type_t server_type, int server_id) {
	server_t *server = (server_t *)malloc(sizeof(*server));
	server->scene = NULL;
	server->server_id = server_id;
	server->clients = list_init(0, NULL);
	server->server_state = SERVER_EMPTY;
	server->server_type = server_type;
	server->start_wait_time.tv_sec = INT_MAX;
	server->game_start = false;
	return server;
}


void server_init_client(server_t *server, client_t *client, size_t idx) {
	printf("initializing client\n");
	scene_t *scene = server_get_scene(server);
	car_t *car = client_get_car(client);
	car_set_position(car, map_get_spawn(server_get_map(server), idx));
	scene_add_body(scene, car_get_body(car));
}

void server_game_start(server_t *server) {
	printf("starting server game\n");
	server->game_start = true;
	scene_t *scene = server_get_scene(server);

	for (size_t i = 0; i < list_size(server->clients); ++i) {
		client_t *client = list_get(server->clients, i);
		car_t *car = client_get_car(client);

		size_t body_count = scene_bodies(scene);
		for (size_t j = 0; j < body_count; ++j) {
			body_t *body = scene_get_body(scene, j);
			switch (((collision_ref_t *)body_get_info(body))->body_type) {
				case WALL: {
					create_car_wall_collision(scene, car_get_elasticity_wall(car),
						car_get_collision_friction(car), car_get_body(car), body);
					break;
				}
				case GRASS: {
					create_car_grass_collision(scene, car_get_body(car), body);
					break;
				}
				case FINISH_BEGIN: {
					create_car_finish_begin_collision(scene, client, car_get_body(car), body);
					break;
				}
				case FINISH_END: {
					create_car_finish_end_collision(scene, client, car_get_body(car), body);
					break;
				}
				default: break;
			}
		}

		create_dynamic_engine(scene, car_get_power_ptr(car), car_get_body(car));
		create_dynamic_braking(scene, car_get_brake_ptr(car), car_get_body(car));
		create_car_drag(scene, car_get_gamma_ptr(car), car_get_body(car));
		create_friction(scene, car_get_mu_ptr(car), car_get_body(car));

		for (size_t j = 0; j < i; ++j) {
			client_t *opponent_client = list_get(server->clients, j);;
			car_t *opponent_car = client_get_car(opponent_client);

			create_car_car_collision(scene, car_get_elasticity_car(car),
				car_get_body(car), car_get_body(opponent_car));
		}
	}
}

void server_add_client(server_t *server, client_t *client) {
	assert("server is full" && list_size(server->clients) < MAX_PLAYERS);
	printf("adding client %i to server %i\n", client_get_id(client), server->server_id);
	list_add(server->clients, client);
	client_set_server_id(client, server->server_id);
	server->server_state = SERVER_LOBBY;

	if (server_client_count(server) == MIN_PLAYERS) {
		// start the wait time
		printf("min players encountered!\n");
		clock_gettime(CLOCK_MONOTONIC, &server->start_wait_time);
		printf("aa %lld\n", (long long)(server->start_wait_time.tv_sec));
	}

	// move to elsewhere... server_init_client(server, client);
}

void server_remove_client(server_t *server, client_t *client) {
	for (size_t i = 0; i < list_size(server->clients); ++i) {
		if ((client_t*)list_get(server->clients, i) == client) {
			puts("server is removing a client\n");
			list_remove(server->clients, i);
			break;
		}
	}
	if (server_client_count(server) == 0) {
		server->server_state = SERVER_EMPTY;
	}
}

list_t *server_get_clients(server_t *server) {
	return server->clients;
}

void server_set_map(server_t *server, map_t *map) {
	server->map = map;

	// add objects to scene
	// future: extend type_info

	list_t *map_objects = map_get_objects(map);
	for (size_t i = 0; i < list_size(map_objects); ++i) {
		scene_add_body(server->scene, list_get(map_objects, i));
	}
}

map_t *server_get_map(server_t *server) {
	return server->map;
}

size_t server_client_count(server_t *server) { return list_size(server->clients); }

int server_get_id(server_t *server) { return server->server_id; }

void server_set_state(server_t *server, server_state_t state) {
	server->server_state = state;
}
server_state_t server_get_state(server_t *server) {
	return server->server_state;
}
struct timespec server_get_start_wait_time(server_t *server) {
	return server->start_wait_time;
}

void server_start(server_t *server) {
	server->server_state = SERVER_IN_GAME;
	for (size_t i = 0; i < list_size(server->clients); ++i) {
		server_init_client(server, (client_t*)list_get(server->clients, i), i);
		if (map_get_type(server->map) == MAP_ONE) {
			client_get_laps((client_t*)list_get(server->clients, i))->time_remaining = SINGLEPLAYER_INIT_TIME_MAP1;
		}
		else if (map_get_type(server->map) == MAP_TWO) {
			client_get_laps((client_t*)list_get(server->clients, i))->time_remaining = SINGLEPLAYER_INIT_TIME_MAP2;
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &server->last_tick);
}

void server_tick(server_t *server) {
	
	struct timespec current;
	clock_gettime(CLOCK_MONOTONIC, &current);
	double delta = (current.tv_sec - server->last_tick.tv_sec) + ((current.tv_nsec - server->last_tick.tv_nsec) / 1000000000.0);
	// printf("delta: %.9f\n", delta);
	server->last_tick = current;

	// Update time
	laps_t *client_laps = client_get_laps((client_t *)list_get(server->clients, 0));
	client_laps->time_remaining -= delta;

	// Start game if time elapsed exceeds countdown time
	if (!server->game_start) {
		switch (map_get_type(server_get_map(server))) {
			case MAP_ONE: {
				if (client_laps->time_remaining < SINGLEPLAYER_INIT_TIME_MAP1 - COUNTDOWN_TIME) {
					server_game_start(server);
				}
				break;
			}
			case MAP_TWO: {
				if (client_laps->time_remaining < SINGLEPLAYER_INIT_TIME_MAP2 - COUNTDOWN_TIME) {
					server_game_start(server);
				}
				break;
			}
			default: {
				break;
			}
		}
	}

	scene_collide(server->scene, delta);

	// compute car physics for each client
	for (size_t i = 0; i < list_size(server->clients); ++i) {
		client_t *client = (client_t *)list_get(server->clients, i);
		car_t *car = client_get_car(client);
		input_data_t input_data = client_get_input_data(client);
		car_compute_physics(car, &input_data);

		list_t *objects = car_make_tracks(car, &input_data);
		// add tracks to scene
		for (size_t j = 0; j < list_size(objects); ++j) {
			body_t *obj = (body_t *)list_get(objects, j);
			create_timed_remover(server->scene, car_get_track_lasting_time(car), obj);
			scene_add_body(server->scene, obj);
		}
		list_free(objects);
	}

	scene_tick(server->scene, delta);

	// update grass
	for (size_t i = 0; i < list_size(server->clients); ++i) {
		client_t *client = (client_t *)list_get(server->clients, i);
		car_t *car = client_get_car(client);

		((collision_ref_t *)body_get_info(car_get_body(car)))->touch_grass = false;
	}
}

bool gameplay_tick_single(server_t *server) {
	// ends game when time is up
	client_t *client = (client_t *)list_get(server->clients, 0);
	laps_t *client_laps = client_get_laps(client);

	return client_laps->time_remaining < 0.0;
}

int gameplay_tick_multi(server_t *server) {
	//ends game when someone wins
	for (size_t i = 0; i < list_size(server->clients); ++i) {
		client_t *client = (client_t *)list_get(server->clients, i);
		laps_t *client_laps = client_get_laps(client);

		if (client_laps->lap_count > (int)MULTIPLAYER_MAX_LAPS) {
			printf("%i\n", client_laps->lap_count);
			return client_get_id(client);
		}
	}
	return NULL;
}