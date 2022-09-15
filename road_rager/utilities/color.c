#include "color.h"
#include "math_util.h"

rgba_color_t generate_random_color() {
	return (rgba_color_t){ generate_random_double(0, 1), generate_random_double(0, 1),
		generate_random_double(0, 1), 1 };
}

rgba_color_t generate_random_pastel_color() {
	return (rgba_color_t){ generate_random_double(0.7, 1), generate_random_double(0.7, 1),
		generate_random_double(0.7, 1), 1 };
}

rgba_color_t generate_random_bright_color() {
	double array[3];
	int low = generate_random_int(0, 2);
	int high = (low + generate_random_int(0, 1) + 1) % 3;
	array[low] = generate_random_double(0, 0.05);
	array[high] = generate_random_double(0.95, 1);
	array[3 - low - high] = generate_random_int(0, 2) / 2.0;

	return (rgba_color_t){ array[0], array[1], array[2], 1 };
}

rgba_color_t color_set_opacity(rgba_color_t color, float opacity) {
	return (rgba_color_t){ color.r, color.g, color.b, opacity };
}
