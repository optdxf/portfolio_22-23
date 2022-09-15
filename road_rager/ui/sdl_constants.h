#ifndef __SDL_CONSTANTS_H__
#define __SDL_CONSTANTS_H__

typedef enum arrow_key {
	LEFT_ARROW = 1,
	UP_ARROW = 2,
	RIGHT_ARROW = 3,
	DOWN_ARROW = 4
} arrow_key_t;

typedef enum key_event_type { EVENT_KEY_PRESSED, EVENT_KEY_RELEASED } key_event_type_t;
typedef enum mouse_event_type { EVENT_MOUSE_MOVE, EVENT_MOUSE_PRESSED, EVENT_MOUSE_RELEASED } mouse_event_type_t;   // only support left mouse button atm

typedef void (*key_handler_t)(char key, key_event_type_t type, void *data);
typedef void (*mouse_handler_t)(double x, double y, mouse_event_type_t type, void *data);

#endif