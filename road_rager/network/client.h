#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "car.h"
#include "map.h"
#include "input_data.h"
#include <stdbool.h>
#include <stdlib.h>
#include "ws.h"
#include <time.h>

typedef struct client client_t;

typedef struct laps {
	int finish_state; // needs to be signed so no size_t
	int lap_count; // for both single- and multi-player
	int lap_max; // for multi-player
	double time_remaining; // for single-player
} laps_t;

// assume caller assigns client id
client_t *client_init(int client_id, ws_cli_conn_t *conn);
void client_free(client_t *client);

client_t *get_client_from_id(int client_id);
void client_set_id(client_t *client, int id);
int client_get_id(client_t *client);
int client_get_server_id(client_t *client);
void client_set_server_id(client_t *client, int server_id);
input_data_t client_get_input_data(client_t *client);
void client_set_input_left(client_t *client, bool value);
void client_set_input_right(client_t *client, bool value);
void client_set_input_up(client_t *client, bool value);
void client_set_input_down(client_t *client, bool value);
void client_set_input_space(client_t *client, bool value);
ws_cli_conn_t *client_get_ws_con(client_t *client);

car_t *client_get_car(client_t *client);
void client_set_car(client_t *client, car_t *car);

laps_t *client_get_laps(client_t *client);
void client_set_laps(client_t *client, laps_t laps);

map_t *client_get_map(client_t *client);
void client_set_map(client_t *client, map_t *map);

#endif