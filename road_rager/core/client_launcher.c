#include "sdl_constants.h"
#include "client_launcher.h"
#include "sdl_wrapper2.h"
#include "global_constants.h"
#include "server.h"
#include "car.h"
#include "map.h"
#include "ui_handler.h"
#include "packet.h"
#include "img_label.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

//--- MULTIPLAYER / NETWORKING



void register_other_client_cars(state_t *state, double map_scale, start_multi_game_packet_t *packet) {
	for (size_t i = 0; i < packet->client_count; ++i) {
		client_car_info_t *info = (client_car_info_t*)list_get(packet->car_info, i);
		if (info->client_id != client_get_id(state->client)) {
			car_t *car = car_init(info->car_type, map_scale, 0, 0);
			client_car_wrapper_t *car_wrapper = malloc(sizeof(*car_wrapper));
			car_wrapper->car = car;
			car_wrapper->client_id = info->client_id;
			HASH_ADD_INT(state->car_client_hash, client_id, car_wrapper);
			scene_add_body(state->scene, car_get_body(car));
		}
		
	}
}

EM_BOOL onopen(int eventType, const EmscriptenWebSocketOpenEvent *websocketEvent, state_t *state) {
	printf("CLIENT: connection OPENED\n");
	state->status = STATUS_IN_LOBBY;
	car_selection_packet_t packet = (car_selection_packet_t){ CAR_SELECTION, state->car_choice };
	uint64_t size = 0;
	void *buffer = write_packet(&packet, CAR_SELECTION, &size);
	emscripten_websocket_send_binary(state->ws, buffer, size);
	free(buffer);

	state->client = client_init(0, NULL);
	printf("car choice: %i\n", state->car_choice);

	return EM_TRUE;
}
EM_BOOL onerror(int eventType, const EmscriptenWebSocketErrorEvent *websocketEvent, state_t *state) {
	printf("CLIENT: connection errored\n");
	return EM_TRUE;
}
EM_BOOL onclose(int eventType, const EmscriptenWebSocketCloseEvent *websocketEvent, state_t *state) {
	// disconnected
	return EM_TRUE;
}
EM_BOOL onmessage(int eventType, const EmscriptenWebSocketMessageEvent *websocketEvent, state_t *state) {
	// printf("CLIENT: received a message from server\n");

	if (!websocketEvent->isText) {
		// only getting binary
		if (websocketEvent->numBytes <= 0) return false;
		packet_type_t type = (packet_type_t)websocketEvent->data[0];
		packet_t *p = parse_packet((const unsigned char *)websocketEvent->data, websocketEvent->numBytes);
		switch (type) {
			case START_MULTI_GAME: {
				start_multi_game_packet_t *packet = (start_multi_game_packet_t*)p;
				printf("server said to start multi!\n");
				// state->status = STATUS_BEGIN_MULTI_PLAYER;

				client_set_id(state->client, packet->local_client_id);

				map_t *map = map_init(packet->map_type);
				//printf("map of packet is %i\n", packet->map_type);

				register_other_client_cars(state, map_get_scaling(map), packet);

				car_t *car = car_init(state->car_choice, map_get_scaling(map), 0, 0);
				client_set_car(state->client, car);
				scene_add_body(state->scene, car_get_body(car));

				const char *fpath = map_get_image(map);
				// const char *filepath, image_type_t image_type, vector_t pos, double width, anchor_point_t anchor_point, bool visible
				img_label_t *map_img = img_label_init(fpath, IMG_TYPE_PNG, (vector_t){0,SCENE_HEIGHT}, SCENE_WIDTH, ANCHOR_TOP_LEFT, true);
				scene_set_map(state->scene, map_img);


				// server_t *server = server_init(SERVER_LOCAL, 0);
				// state->server = server;
				// server_set_map(server, map); // just need access to the map
				state->map = map;

				state->status = STATUS_IN_GLOBAL_GAME;
				break;
			} case LOBBY_LIST: {


				lobby_list_packet_t *packet = (lobby_list_packet_t*)p;
				while (list_size(state->lobby_ids)) free(list_remove(state->lobby_ids, list_size(state->lobby_ids) - 1));
				list_t *clie = packet->clients;
				printf("cleaned lobby list. size: %zu\n", list_size(state->lobby_ids));
				printf("clie has: %zu\n", list_size(clie));
				size_t size = list_size(clie);
				for (size_t i = 0; i < size; ++i) {
					//printf("adding %zu\n", i);
					list_add(state->lobby_ids, list_remove(clie, list_size(clie) - 1));
				}
				list_free(clie);
				// free(packet);

				//printf("lobby id has %zu elements\n", list_size(state->lobby_ids));



				//printf("got list of lobbies from server\n");
				// lobby_list_packet_t *packet = (lobby_list_packet_t*)p;
				//printf("testing... %zu\n", list_size(packet->clients));
				//i// f (state->lobby_ids) { printf("clearling old\n"); list_free(state->lobby_ids); state->lobby_ids = NULL; };
				//printf("testing2... %zu\n", list_size(packet->clients));
				//state->lobby_ids = packet->clients;
				// free(packet);
				//printf("testing3... %zu\n", list_size(state->lobby_ids));
				break;
			} case ALL_UPDATES: {
				all_car_body_packet_t *packet = (all_car_body_packet_t*)p;
				for (size_t i = 0; i < packet->client_count; ++i) {
					car_body_data_t *data = list_get(packet->car_body_data, i);
					if (data->client_id == client_get_id(state->client)) {
						//printf("our car!\n");
						car_t *car = client_get_car(state->client);
						car_set_position(car, (vector_t){data->x, data->y});
						body_t *body = car_get_body(car);
						body_set_rotation(body, data->angle);
					} else {
						client_car_wrapper_t *car_wrapper;
						HASH_FIND_INT(state->car_client_hash, &data->client_id, car_wrapper);
						car_t *car = car_wrapper->car;
						car_set_position(car, (vector_t){data->x, data->y});
						body_t *body = car_get_body(car);
						body_set_rotation(body, data->angle);
					}
					//printf("%i\n", data->client_id);
				}
				// printf("getting updates :)\n");
				// printf("%i\n", packet->client_count);
				break;
			} case UPDATE_ID: {
				update_client_id_packet_t *packet = (update_client_id_packet_t*)p;
				client_set_id(state->client, packet->id);
				break;
			} case UPDATE_LAPS: {
				update_lap_counter_packet_t *packet = (update_lap_counter_packet_t*)p;
				client_set_laps(state->client, (laps_t){client_get_laps(state->client)->finish_state, packet->lap_count, packet->lap_max, 0});
				break;
			} case MULTI_WINNER: {
				state->status = STATUS_MULTI_PLAYER_GAME_OVER;
				multi_winner_packet_t *packet = (multi_winner_packet_t*)p;
				state->multi_winner = packet->id;
				break;
			}
		}
		free(p);
	}
	
	return EM_TRUE;
}

//-----


void sdl_key_handler(char key, key_event_type_t type, state_t *state) {
	// ignore all keys except arrows
	if (!(key == LEFT_ARROW || key == RIGHT_ARROW || key == DOWN_ARROW ||
			key == UP_ARROW || key == ' ' || key == 'w' || key == 'a' || key == 's' ||
			key == 'd')) {
		return;
	}
	// ignore keys unless we have a client obj
	if (!state->client)
		return;

	if (type == EVENT_KEY_PRESSED) {
		// new key pressed
		switch (key) {
			case ' ': {
				client_set_input_space(state->client, true);
				break;
			}
			case LEFT_ARROW: {
				client_set_input_left(state->client, true);
				break;
			}
			case RIGHT_ARROW: {
				client_set_input_right(state->client, true);
				break;
			}
			case DOWN_ARROW: {
				client_set_input_down(state->client, true);
				break;
			}
			case UP_ARROW: {
				client_set_input_up(state->client, true);
				break;
			}
			case 'w': {
				client_set_input_up(state->client, true);
				break;
			}
			case 'a': {
				client_set_input_left(state->client, true);
				break;
			}
			case 's': {
				client_set_input_down(state->client, true);
				break;
			}
			case 'd': {
				client_set_input_right(state->client, true);
				break;
			}
		}
	} else if (type == EVENT_KEY_RELEASED) {
		switch (key) {
			case ' ': {
				client_set_input_space(state->client, false);
				break;
			}
			case LEFT_ARROW: {
				client_set_input_left(state->client, false);
				break;
			}
			case RIGHT_ARROW: {
				client_set_input_right(state->client, false);
				break;
			}
			case DOWN_ARROW: {
				client_set_input_down(state->client, false);
				break;
			}
			case UP_ARROW: {
				client_set_input_up(state->client, false);
				break;
			}
			case 'w': {
				client_set_input_up(state->client, false);
				break;
			}
			case 'a': {
				client_set_input_left(state->client, false);
				break;
			}
			case 's': {
				client_set_input_down(state->client, false);
				break;
			}
			case 'd': {
				client_set_input_right(state->client, false);
				break;
			}
		}
	}
}

void send_key_updates(state_t *state) {
	client_input_packet_t packet = (client_input_packet_t) {
		CLIENT_INPUT, client_get_input_data(state->client)
	};
	uint64_t size;
	void *buffer = write_packet(&packet, CLIENT_INPUT, &size);
	emscripten_websocket_send_binary(state->ws, buffer, (uint32_t)size);
	free(buffer);
}

void core_loop(state_t *state) {
	ui_update();
	if (state->status != STATUS_IN_LOCAL_GAME && state->status != STATUS_IN_GLOBAL_GAME) {
		sdl_render_scene_with_last(state->scene, NULL);
	} else {
		double map_scale = map_get_scaling(state->map);
		sdl_render_partial_scene_with_last(state->scene, car_get_body(client_get_car(state->client)),
			map_scale * CAMERA_WIDTH, map_scale * CAMERA_HEIGHT);
	}
	//("STATUS OF GAME: %i\n", state->status);
	if (state->status == STATUS_BEGIN_MULTI_PLAYER) {
		//printf("a\n");
		// open a connection
		state->status = STATUS_OPENING_CONNECTION;
		EmscriptenWebSocketCreateAttributes ws_attrs = { "ws://labradoodle.caltech.edu:" WEBSOCKET_PORT_STR, NULL, EM_TRUE };
		state->ws = emscripten_websocket_new(&ws_attrs);

		emscripten_websocket_set_onopen_callback(state->ws, state, (em_websocket_open_callback_func)onopen);
		emscripten_websocket_set_onerror_callback(state->ws, state, (em_websocket_error_callback_func)onerror);
		emscripten_websocket_set_onclose_callback(state->ws, state, (em_websocket_close_callback_func)onclose);
		emscripten_websocket_set_onmessage_callback(state->ws, state, (em_websocket_message_callback_func)onmessage);
	} else if (state->status == STATUS_BEGIN_SINGLE_PLAYER) {
		//printf("b\n");
		// generate car, map, server & set variables
		printf("STARTING LOCAL SERVER\n");
		state->client = client_init(0, NULL);
		map_t *map = map_init(state->map_choice);
		img_label_t *map_img = img_label_init(map_get_image(map), IMG_TYPE_PNG, (vector_t){0, SCENE_HEIGHT}, SCENE_WIDTH, ANCHOR_TOP_LEFT, true);
		car_t *car = car_init(state->car_choice, map_get_scaling(map), 0, 0);
		client_set_car(state->client, car);
		client_set_map(state->client, map);
		state->map = map;

		scene_set_map(state->scene, map_img);
		server_t *server = server_init(SERVER_LOCAL, 0);
		state->server = server;
		server_set_scene(server, state->scene);
		server_set_map(server, map);
		server_add_client(server, state->client);
		
		server_start(server);
		state->status = STATUS_IN_LOCAL_GAME;
	} else if (state->status == STATUS_IN_LOCAL_GAME) {
		//printf("c\n");
		server_tick(state->server);
		if (gameplay_tick_single(state->server)) {
			state->status = STATUS_SINGLE_PLAYER_GAME_OVER;
		}
	} else if (state->status == STATUS_IN_GLOBAL_GAME) {
		send_key_updates(state);
	}

	/*
	static bool done = false;
	emscripten_websocket_get_ready_state(ws, &ready_state);
	if (ready_state && !done) {
		done = true;

		
		client_input_packet_t test = (client_input_packet_t) { CLIENT_INPUT, {1, 1, 0, 0} };
		int64_t size = 0;
		void *result = write_packet(&test, CLIENT_INPUT, &size);
		emscripten_websocket_send_binary(ws, result, (uint32_t)size);
	};*/
}

void sdl_mouse_handler(double x, double y, mouse_event_type_t type, state_t *state) {
	// process UI
	ui_group_t *group = scene_get_ui(state->scene);
	if (!group) return;
	ui_group_process_mouse_event(group, x, y, type);
}

void state_free() {
	// scene_Free
}

state_t *state_init() {
	//packet_tests();

	
	state_t *state = malloc(sizeof(*state));
	state->scene = scene_init();
	state->client = NULL;
	state->status = STATUS_MENU;
	state->mode = SINGLE;	// doesn't matter
	state->car_choice = 0;
	state->map_choice = 0;
	state->car_client_hash = NULL;
	state->lobby_ids = list_init(0, free);
	state->ws = 0;

	// car = car_init(CAR_ONE, 100, 100);
	//scene_add_body(state->scene, car_get_body(car));

	//scene_set_ui(state->scene, get_main_menu());
	//scene_set_map(state->scene, get_bkgd());

	return state;
}

//-- EMSCRIPTEN

void emscripten_loop() {
	static bool initialized = false;
	static state_t *state = NULL;
	if (!initialized) {
		initialized = true;
		state = state_init();

		sdl_init((vector_t){ SCENE_WIDTH, SCENE_HEIGHT });
		sdl_set_mouse_handler((mouse_handler_t)sdl_mouse_handler);
		sdl_set_key_handler((key_handler_t)sdl_key_handler);
		init_ui_handler(state);
		show_main_menu();

		// test
		// EmscriptenWebSocketCreateAttributes ws_attrs = { "ws://labradoodle.caltech.edu:" WEBSOCKET_PORT_STR, NULL, EM_TRUE };
		// ws = emscripten_websocket_new(&ws_attrs);

		// emscripten_websocket_set_onopen_callback(ws, NULL, onopen);
		// emscripten_websocket_set_onerror_callback(ws, NULL, onerror);
		// emscripten_websocket_set_onclose_callback(ws, NULL, onclose);
		// emscripten_websocket_set_onmessage_callback(
		// 	ws, state, (em_websocket_message_callback_func)onmessage);
		// state->ws = ws;
	}

	core_loop(state);

	// process keyboard/mouse events & exit if SDL is quitting
	if (sdl_is_done(state)) {
		emscripten_cancel_main_loop();
		state_free();
		emscripten_force_exit(0);
	}
}

int main() {
	emscripten_set_main_loop_arg(emscripten_loop, NULL, 0, 1);
}