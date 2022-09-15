#ifndef __INPUT_H__
#define __INPUT_H__

#include <stdbool.h>

typedef struct input_data {
	bool left_held;
	bool right_held;
	bool up_held;
	bool down_held;
	bool space_held;
} input_data_t;

#endif