#define _POSIX_C_SOURCE 199309L

#include "global_constants.h"
#include "server.h"
#include "ws.h"
#include "uthash.h"
#include "queue.h"
#include "math_util.h"
#include "packet.h"
#include "car.h"
#include "map.h"
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

typedef struct server_wrapper {
	server_t *server;
	int server_id;
	UT_hash_handle hh;
} server_wrapper_t;

server_wrapper_t *servers = NULL;
queue_t *server_q = NULL;
int server_count = 0;

// the assumption is that servers persist; they are not ''closed''
// once a server is created, it stays alive, but it can change states (e.g. unused, used,
// etc.)
void server_find(client_t *client) {
	printf("finding server\n");
	if (queue_empty(server_q)) {
		int new_server_id = ++server_count;
		server_t *server = server_init(SERVER_GLOBAL, new_server_id);
		scene_t *scene = scene_init();
		server_set_scene(server, scene);
		//map_t *map = map_init(generate_random_int(0, 1) == 1 ? MAP_ONE : MAP_TWO);
		map_t *map = map_init(MAP_ONE);
		server_set_map(server, map);

		server_wrapper_t *wrapper = (server_wrapper_t*)malloc(sizeof(*wrapper));
		wrapper->server = server;
		wrapper->server_id = new_server_id;
		HASH_ADD_INT(servers, server_id, wrapper);
		queue_enqueue(server_q, server);
	}
		
	server_t *server = queue_front(server_q);
	server_add_client(server, client);

	puts("done\n");

	if (server_client_count(server) >= MAX_PLAYERS) {
		printf("[server %i is at max capacity, automatically starting the game]\n", server_get_id(server));
		// remove this server from queue
		queue_dequeue(server_q);
		// set status to start
		server_set_state(server, SERVER_READY_TO_START);
	}
}

server_t *get_server_from_id(int id) {
	server_wrapper_t *wrapper;
    HASH_FIND_INT(servers, &id, wrapper);
	if (wrapper) {
		return wrapper->server;
	} else {
		return NULL;
	}
}

//-- WEBSOCKET
void onopen(ws_cli_conn_t *conn) {
	// Open connection -> request to join a server (so no need for a packet)
	int client_id = ws_getclientsock(conn);
	printf("[player request to join a server] id: %i\n", client_id);
	client_t *client = client_init(client_id, conn);
	update_client_id_packet_t packet = (update_client_id_packet_t) { UPDATE_ID, client_id };
	uint64_t size;
	void *buffer = write_packet(&packet, UPDATE_ID, &size);
	ws_sendframe_bin(conn, buffer, size);
	free(buffer);
	//server_find(client);

	//map_t *map = server_get_map(global_server);
	// vector_t map_spawn = map_get_spawn(map);
	// car_t *car = car_init(CAR_ONE, map_spawn.x, map_spawn.y);
	// client_set_car(client, car);
}

void onclose(ws_cli_conn_t *conn) {
	puts("onclose");
	client_t *client = get_client_from_id(ws_getclientsock(conn));
	if (!client) return;

	// client either disconnected or game is over
	int server_id = client_get_server_id(client);
	//printf("%i\n", server_id);
	server_t *server = get_server_from_id(server_id);
	if (server) {
		server_remove_client(server, client);
	}
	

	// broadcast to all clients that a player has left
}

void onmessage(ws_cli_conn_t *conn, const unsigned char *msg, uint64_t size, int type) {
	// puts("msg");

	if (type != WS_FR_OP_BIN) return;	// only taking in binary atm

	client_t *client = get_client_from_id(ws_getclientsock(conn));
	// server_t *server = get_server_from_id(client_get_server_id(client));
	// map_t *map = server_get_map(server);
	if (!client) return;
	

	// printf("client %i | packet size: %" PRIu64 "\n", client_get_id(client), size);
	//printf("client %i | packet data: %s\n", client_get_id(client), msg);

	packet_t *p = parse_packet(msg, size);
	if (p) {
		// printf("parsed correctly!\n");
		switch (p->type) {
			case CLIENT_INPUT: {
				client_input_packet_t *packet = (client_input_packet_t*)p;
				client_set_input_down(client, packet->data.down_held);
				client_set_input_up(client, packet->data.up_held);
				client_set_input_left(client, packet->data.left_held);
				client_set_input_right(client, packet->data.right_held);
				client_set_input_space(client, packet->data.space_held);
				// printf("got inptu!\n");
				break;
			}
			case CAR_SELECTION: {
				car_selection_packet_t *packet = (car_selection_packet_t*)p;
				

				server_find(client);
				server_t *server = get_server_from_id(client_get_server_id(client));
				map_t *map = server_get_map(server);
				// vector_t map_spawn = map_get_spawn(map);

				printf("car type: %i\n", packet->car_type);
				car_t *car = car_init(packet->car_type, map_get_scaling(map), 0, 0);
				client_set_car(client, car);
				client_set_map(client, map);

				// body_set_centroid(car_get_body(car), map_spawn);
				
				
				break;
			}
			default: break;
		}
		free(p);
	}
}

void loop() {
	while (true) {
		for (server_wrapper_t *s = servers; s != NULL; s = s->hh.next) {
			server_t *server = s->server;
			switch (server_get_state(server)) {
				case SERVER_LOBBY: {
					struct timespec current;
					clock_gettime(CLOCK_MONOTONIC, &current);

					struct timespec prev = server_get_start_wait_time(server);
					double delta = (current.tv_sec - prev.tv_sec) + ((current.tv_nsec - prev.tv_nsec) / 1000000000.0);
					//printf("sending packet data\n");

					list_t *clients = server_get_clients(server);
					list_t *client_data = list_init(list_size(clients), free);
					for (size_t i = 0; i < list_size(clients); ++i) {
						client_t *client = (client_t*)list_get(clients, i);
						client_lobby_info_t *info = malloc(sizeof(*info));
						info->client_id = client_get_id(client);
						list_add(client_data, info);
					}
					//printf("client_data: %zu\n", list_size(client_data));
					lobby_list_packet_t packet = (lobby_list_packet_t) {
						LOBBY_LIST, list_size(clients), client_data
					};
					uint64_t size;
					void *buffer = write_packet(&packet, LOBBY_LIST, &size);
					list_free(client_data);
					for (size_t i = 0; i < list_size(clients); ++i) {
						//printf("client: %i\n", )
						client_t *client = (client_t*)list_get(clients, i);
						ws_cli_conn_t *conn = client_get_ws_con(client);
						ws_sendframe_bin(conn, buffer, size);
					}
					free(buffer);

					if (server_client_count(server) >= MIN_PLAYERS && delta >= LOBBY_TIME) {
						printf("aa %lld\n", prev.tv_sec);
						printf("del %.9f\n", delta);
						printf("lobby -> server_ready_to_start\n");
						queue_dequeue(server_q);
						server_set_state(server, SERVER_READY_TO_START);
					}
					break;
				}
				case SERVER_READY_TO_START: {
					// broadcast to all clients we are ready to begin
					// get all clients' cars
					printf("STARTING SERVER!\n");
					list_t *clients = server_get_clients(server);
					list_t *client_info = list_init(list_size(clients), free);
					for (size_t i = 0; i < list_size(clients); ++i) {
						client_t *client = (client_t*)list_get(clients, i);
						client_car_info_t *info = malloc(sizeof(*info));
						info->client_id = client_get_id(client);
						info->car_type = car_get_type(client_get_car(client));
						list_add(client_info, info);
					}
					
					printf("transmitting\n");
					for (size_t i = 0; i < list_size(clients); ++i) {
						client_t *client = (client_t*)list_get(clients, i);
						ws_cli_conn_t *conn = client_get_ws_con(client);
						start_multi_game_packet_t packet = (start_multi_game_packet_t){ START_MULTI_GAME, map_get_type(server_get_map(server)), (int)list_size(clients), client_get_id(client), client_info };
						uint64_t size = 0;
						void *buffer = write_packet(&packet, START_MULTI_GAME, &size);
						ws_sendframe_bin(conn, buffer, size);
						free(buffer);
						
					}
					
					list_free(client_info);
					server_set_state(server, SERVER_START);
					break;
				}
				case SERVER_START: {
					server_start(server);
					break;
				}
				case SERVER_IN_GAME: {
					server_tick(server);
					int winner = gameplay_tick_multi(server); // returns boolean; if true, game is over
					// transmit

					if (winner) {
						printf("someone won!\n");

						server_set_state(server, SERVER_DONE);
						multi_winner_packet_t packet = (multi_winner_packet_t) { MULTI_WINNER, winner };
						uint64_t size;
						void *buffer = write_packet(&packet, MULTI_WINNER, &size);
						list_t *clients = server_get_clients(server);
						for (size_t i = 0; i < list_size(clients); ++i) {
							client_t *client = (client_t*)list_get(clients, i);
							ws_cli_conn_t *conn = client_get_ws_con(client);
							ws_sendframe_bin(conn, buffer, size);
						}
						free(buffer);
						break;
					}


					list_t *clients = server_get_clients(server);
					list_t *client_info = list_init(list_size(clients), free);

					for (size_t i = 0; i < list_size(clients); ++i) {
						client_t *client = (client_t*)list_get(clients, i);
						car_body_data_t *info = malloc(sizeof(*info));
						body_t *car_body = car_get_body(client_get_car(client));
						vector_t pos = body_get_centroid(car_body);

						info->client_id = client_get_id(client);
						info->x = pos.x;
						info->y = pos.y;
						info->angle = body_get_rotation(car_body);

						list_add(client_info, info);
					}
					all_car_body_packet_t packet = (all_car_body_packet_t){ ALL_UPDATES, list_size(clients), client_info };
					uint64_t size = 0;
					void *buffer = write_packet(&packet, ALL_UPDATES, &size);
					for (size_t i = 0; i < list_size(clients); ++i) {
						client_t *client = (client_t*)list_get(clients, i);
						ws_cli_conn_t *conn = client_get_ws_con(client);
						ws_sendframe_bin(conn, buffer, size);
						sleep(.001);

						laps_t *laps = client_get_laps(client);
						update_lap_counter_packet_t packet = (update_lap_counter_packet_t) { UPDATE_LAPS, laps->lap_count, laps->lap_max };
						uint64_t size;
						void *lap_buffer = write_packet(&packet, UPDATE_LAPS, &size);
						ws_sendframe_bin(conn, lap_buffer, size);
						free(lap_buffer);
					}
					free(buffer);
					list_free(client_info);
					/*
					list_t *clients = server_get_clients(server);
					for (size_t i = 0; i < list_size(clients); ++i) {
						client_t *client = (client_t*)list_get(clients, i);
						ws_cli_conn_t *conn = client_get_ws_con(client);

						start_multi_game_packet_t packet = (start_multi_game_packet_t){ START_MULTI_GAME, (int)list_size(clients), client_info };
						uint64_t size = 0;
						void *buffer = write_packet(&packet, START_MULTI_GAME, &size);
						ws_sendframe_bin(conn, buffer, size);
						free(buffer);
					}*/
					break;
				}
				default: {
					break;
				}
			}
		}
		sleep(.001);
	}
}

int main() {
	srand(time(NULL));
	server_q = queue_init(NULL);

	struct ws_events evs;
	evs.onopen = &onopen;
	evs.onclose = &onclose;
	evs.onmessage = &onmessage;

	pthread_t loop_thread;
	if (pthread_create(&loop_thread, NULL, (void *(*)(void *))loop, NULL)) {
		puts("Could not create the server thread!");
		return 0;
	}
	pthread_detach(loop_thread);
	ws_socket(&evs, WEBSOCKET_PORT, 1, 1000);
	system("python3 -m http.server " FILE_SERVING_PORT_STR);
	return 0;
}