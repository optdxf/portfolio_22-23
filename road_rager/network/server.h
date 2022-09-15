#ifndef __SERVER_H__
#define __SERVER_H__

#include "client.h"
#include <time.h>
#include "map.h"
#include "scene.h"

typedef struct server server_t;
typedef enum server_type { SERVER_LOCAL, SERVER_GLOBAL } server_type_t;
typedef enum server_state { SERVER_EMPTY, SERVER_LOBBY, SERVER_READY_TO_START,
    SERVER_START, SERVER_IN_GAME, SERVER_DONE } server_state_t;

const extern double SINGLEPLAYER_INIT_TIME_MAP1;
const extern double SINGLEPLAYER_INIT_TIME_MAP2;
const extern size_t MULTIPLAYER_MAX_LAPS;
extern double singleplayer_extra_time(int lap_count, map_t *map);

void server_init_client(server_t *server, client_t *client, size_t idx);
void server_game_start(server_t *server);
server_t *server_init(server_type_t server_type, int server_id);
size_t server_client_count(server_t *server);
int server_get_id(server_t *server);
void server_add_client(server_t *server, client_t *client);
void server_remove_client(server_t *server, client_t *client);
list_t *server_get_clients(server_t *server);
void server_set_state(server_t *server, server_state_t state);
server_state_t server_get_state(server_t *server);
struct timespec server_get_start_wait_time(server_t *server);
void server_start(server_t *server);
void server_tick(server_t *server);

/**
* @return whether the single player game is over
*/
bool gameplay_tick_single(server_t *server);


/**
* @return The player that one
*/
int gameplay_tick_multi(server_t *server);


/**
* @return pointer to the server map
*/
map_t *server_get_map(server_t *server);


/**
* sets map for server
*/
void server_set_map(server_t *server, map_t *map);

/**
* sets scene for server
*/
void server_set_scene(server_t *server, scene_t *scene);

#endif