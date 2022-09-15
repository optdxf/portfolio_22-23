#ifndef __CLIENT_LAUNCHER_H__
#define __CLIENT_LAUNCHER_H__

#include "scene.h"
#include "client.h"
#include "map.h"
#include "car.h"
#include "server.h"
#include "uthash.h"
#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>

typedef enum ui_game_mode_choice { SINGLE, MULTI }  ui_game_mode_choice_t;
typedef enum game_status { STATUS_BEGIN_SINGLE_PLAYER, STATUS_BEGIN_MULTI_PLAYER, STATUS_OPENING_CONNECTION, STATUS_IN_LOBBY, STATUS_MENU, STATUS_IN_LOCAL_GAME, STATUS_IN_GLOBAL_GAME, STATUS_SINGLE_PLAYER_GAME_OVER, STATUS_MULTI_PLAYER_GAME_OVER } game_status_t;

typedef struct server_wrapper {
	server_t *server;
	int server_id;
	UT_hash_handle hh;
} server_wrapper_t;

typedef struct client_car_wrapper {
	int client_id;
    car_t *car;
	UT_hash_handle hh;
} client_car_wrapper_t;

typedef struct state {
	scene_t *scene;
    server_t *server;
    client_t *client;
    client_car_wrapper_t *car_client_hash;
	game_status_t status;
    ui_game_mode_choice_t mode;
    car_type_t car_choice;
    map_type_t map_choice;
    list_t *lobby_ids;
    map_t *map;
    int multi_winner;
    EMSCRIPTEN_WEBSOCKET_T ws;

} state_t;

#endif