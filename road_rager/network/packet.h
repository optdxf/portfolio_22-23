#ifndef __PACKET_H__
#define __PACKET_H__

#include "input_data.h"
#include "list.h"
#include "car.h"
#include "map.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum packet_type { CLIENT_INPUT, LOBBY_LIST, START_MULTI_GAME, CAR_SELECTION, ALL_UPDATES, UPDATE_ID, UPDATE_LAPS, MULTI_WINNER } packet_type_t;

typedef struct packet {
	packet_type_t type;
} packet_t;

typedef struct car_body_data {
	int client_id;
	double x;
	double y;
	double angle;
} car_body_data_t;

typedef struct all_car_body_packet {
	packet_type_t type;
	int client_count;
	list_t *car_body_data;
} all_car_body_packet_t;

typedef struct client_input_packet {
	packet_type_t type;
	input_data_t data;
} client_input_packet_t;

typedef struct client_lobby_info {
	int client_id;
} client_lobby_info_t;

typedef struct client_car_info {
	int client_id;
	car_type_t car_type;
} client_car_info_t;

typedef struct start_multi_game_packet {
	packet_type_t type;
	map_type_t map_type;
	int client_count;
	int local_client_id;
	list_t *car_info;
} start_multi_game_packet_t;

typedef struct lobby_input_packet {
	packet_type_t type;
	int client_count;
	list_t *clients;
} lobby_list_packet_t;

typedef struct car_selection_packet {
	packet_type_t type;
	car_type_t car_type;
} car_selection_packet_t;

typedef struct update_client_id_packet {
	packet_type_t type;
	int id;
} update_client_id_packet_t;

typedef struct update_lap_counter_packet {
	packet_type_t type;
	int lap_count;
	int lap_max;
} update_lap_counter_packet_t;

typedef struct multi_winner_packet {
	packet_type_t type;
	int id;
} multi_winner_packet_t;


packet_t *parse_packet(const unsigned char *msg, uint64_t size);
const unsigned char *write_packet(packet_t *p, packet_type_t type, uint64_t *size);
void packet_tests();
#endif