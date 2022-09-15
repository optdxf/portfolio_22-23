#ifndef __COLOR_H__
#define __COLOR_H__

/**
 * A color to display on the screen.
 * The color is represented by its red, green, and blue components, and an alpha value.
 * Each component must be between 0 (black) and 1 (white).
 * Alpha channel must be between 0 (transparent) and 1 (opaque).
 */
typedef struct {
	float r;
	float g;
	float b;
	float a;
} rgba_color_t;

/**
 * Generates a random color with opacity of 1.
 */
rgba_color_t generate_random_color();

/**
 * Generates a random pastel color with opacity of 1.
 */
rgba_color_t generate_random_pastel_color();

/**
 * Generates a random bright color with opcaity of 1.
 */
rgba_color_t generate_random_bright_color();

/**
 * Set the opacity of a color by changing the alpha channel value.
 * @param color input color
 * @param opacity alpha value to set
 */
rgba_color_t color_set_opacity(rgba_color_t color, float opacity);

#endif
