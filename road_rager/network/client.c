#include "client.h"
#include "uthash.h"
#include <time.h>
#include <stdio.h>
#include <math.h>

struct client {
	int client_id; // not size_t b/c it depends on websocket library, which uses int
	int server_id;
	input_data_t input;
	car_t *car;
	map_t *map;
	laps_t *laps;
	ws_cli_conn_t *conn;
	UT_hash_handle hh; // for hashmap purposes
};

client_t *clients = NULL; // dictionary of clients [map client_id -> client]

client_t *client_init(int client_id, ws_cli_conn_t *conn) {
	client_t *client = malloc(sizeof(*client));
	client->client_id = client_id;
	client->server_id = 0;
	client->input = (input_data_t){};
	client->car = NULL;
	client->conn = conn;
	client->laps = malloc(sizeof(*client->laps));
	client->laps->finish_state = 2;
	client->laps->lap_count = -1;
	client->laps->lap_max = 0;
	HASH_ADD_INT(clients, client_id, client);
	return client;
}

void client_set_server_id(client_t *client, int server_id) {
	client->server_id = server_id;
}

map_t *client_get_map(client_t *client) {
	return client->map;
}

void client_set_map(client_t *client, map_t *map) {
	client->map = map;
}

void client_free(client_t *client) {
	HASH_DEL(clients, client);
	free(client);
}

int client_get_id(client_t *client) { return client->client_id; }

client_t *get_client_from_id(int client_id) {
	for (client_t *c = clients; c != NULL; c = c->hh.next) {
		if (c->client_id == client_id) {
			return c;
		}
	}
	return NULL;
}

ws_cli_conn_t *client_get_ws_con(client_t *client) {
	return client->conn;
}

int client_get_server_id(client_t *client) { return client->server_id; }

void client_set_id(client_t *client, int id) { client->client_id = id; }

input_data_t client_get_input_data(client_t *client) { return client->input; }

void client_set_input_left(client_t *client, bool value) {
	client->input.left_held = value;
}
void client_set_input_right(client_t *client, bool value) {
	client->input.right_held = value;
}
void client_set_input_up(client_t *client, bool value) {
	client->input.up_held = value;
	}
void client_set_input_down(client_t *client, bool value) {
	client->input.down_held = value;
}
void client_set_input_space(client_t *client, bool value) {
	client->input.space_held = value;
}

car_t *client_get_car(client_t *client) { return client->car; }

void client_set_car(client_t *client, car_t *car) { client->car = car; }

laps_t *client_get_laps(client_t *client) { return client->laps; }

void client_set_laps(client_t *client, laps_t laps) { *(client->laps) = laps; }