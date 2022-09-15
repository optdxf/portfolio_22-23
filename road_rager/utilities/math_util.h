/**
 * Library of short math functions.
 */

#ifndef __MATH_UTIL_H__
#define __MATH_UTIL_H__

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

static inline int min_int(int a, int b) { return a < b ? a : b; }

static inline double min_dbl(double a, double b) { return a < b ? a : b; }

static inline int max_int(int a, int b) { return a > b ? a : b; }

static inline double max_dbl(double a, double b) { return a > b ? a : b; }

static inline double clamp_dbl(double x, double min, double max) {
	if (x < min) {
		return min;
	} else if (x > max) {
		return max;
	} else {
		return x;
	}
}

static inline bool between_dbl(double x, double min, double max) {
	return x >= min && x <= max;
}

// min and max are inclusive
static inline int generate_random_int(int min, int max) {
	return rand() % (max - min + 1) + min;
}

// min and max are inclusive
static inline double generate_random_double(double min, double max) {
	return min + (rand() / (RAND_MAX / (max - min)));
}

static bool within(double epsilon, double d1, double d2) {
	return fabs(d1 - d2) < epsilon;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
static bool isclose(double d1, double d2) { return within(1e-7, d1, d2); }
#pragma clang diagnostic pop


#endif
