#ifndef __GLOBAL_CONSTANTS_H__
#define __GLOBAL_CONSTANTS_H__

// Makefile will define these; we add these here to avoid warnings
#ifndef FILE_SERVING_PORT_STR
#define FILE_SERVING_PORT_STR "0000"
#endif
#ifndef WEBSOCKET_PORT
#define WEBSOCKET_PORT 0000
#endif
#ifndef WEBSOCKET_PORT_STR
#define WEBSOCKET_PORT_STR "0000"
#endif

#include <stdlib.h>

// Define these in the .c file
extern const double SCENE_WIDTH;
extern const double SCENE_HEIGHT;
extern const double WINDOW_WIDTH;
extern const double WINDOW_HEIGHT;
extern const double CAMERA_WIDTH;
extern const double CAMERA_HEIGHT;
extern const char *GAME_NAME;

extern const size_t MAX_PLAYERS;
extern const size_t MIN_PLAYERS;
extern const double LOBBY_TIME;

#endif