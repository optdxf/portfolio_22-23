#include "vector.h"
#include "math_util.h"
#include <math.h>

const vector_t VEC_ZERO = {};

vector_t vec_add(vector_t v1, vector_t v2) {
	return (vector_t){ v1.x + v2.x, v1.y + v2.y };
}

vector_t vec_subtract(vector_t v1, vector_t v2) { return vec_add(v1, vec_negate(v2)); }

vector_t vec_negate(vector_t v) { return vec_multiply(-1, v); }

vector_t vec_multiply(double scalar, vector_t v) {
	return (vector_t){ v.x * scalar, v.y * scalar };
}

double vec_dot(vector_t v1, vector_t v2) { return v1.x * v2.x + v1.y * v2.y; }

double vec_cross(vector_t v1, vector_t v2) { return v1.x * v2.y - v1.y * v2.x; }

vector_t vec_rotate(vector_t v, double angle) {
	return (vector_t){ cos(angle) * v.x - sin(angle) * v.y,
		sin(angle) * v.x + cos(angle) * v.y };
}

vector_t vec_rotate_about(vector_t v, vector_t about, double angle) {
	vector_t diff = vec_subtract(v, about);
	diff = vec_rotate(diff, angle);
	return vec_add(about, diff);
}

double vec_magnitude(vector_t v) { return sqrt(vec_dot(v, v)); }

vector_t vec_unit_vec(vector_t v) { return vec_multiply(1 / vec_magnitude(v), v); }

double vec_angle_between(vector_t v1, vector_t v2) {
	double v1v2 = vec_magnitude(v1) * vec_magnitude(v2);
	if (v1v2 == 0) {
		return 0;
	}
	double angle = acos(vec_dot(v1, v2) / v1v2);
	if (vec_dot(v1, vec_rotate(v2, M_PI / 2)) < 0) {
		angle = 2 * M_PI - angle;
	}
	return angle;
}

bool vec_within(double epsilon, vector_t v1, vector_t v2) {
	return within(epsilon, v1.x, v2.x) && within(epsilon, v1.y, v2.y);
}

bool vec_equal(vector_t v1, vector_t v2) { return v1.x == v2.x && v1.y == v2.y; }

bool vec_isclose(vector_t v1, vector_t v2) {
	return isclose(v1.x, v2.x) && isclose(v1.y, v2.y);
}