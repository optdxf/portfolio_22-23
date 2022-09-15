#ifndef __TEXT_LABEL_H__
#define __TEXT_LABEL_H__

#include <SDL2/SDL.h>
#include "vector.h"
#include "color.h"
#include <stdbool.h>

/**
 * Text label is a framework for displaying text in the UI. It contains a message, font, position
 * for the text, fontsize, a boolean denoting whether it is visible, and the color of the text.
**/
typedef struct text_label text_label_t;

/**
 * Initializes the text label
 * @param message text of the text_label
 * @param font path of the font
 * @param position upper-left corner of the text
 * @param fontsize
 * @param visible whether the text is to be visible
 * @param color
**/
text_label_t* text_label_init(char* message, char *font, vector_t position, int fontsize, bool visible, rgba_color_t color);

/**
 * Gets the visibility of the text label
**/
bool text_label_get_visible(text_label_t *text);

/**
 * Gets the message of the text label
**/
char* text_label_get_message(text_label_t *text);

/**
 * Gets the position of the text label
**/
vector_t text_label_get_position(text_label_t *text);

/**
 * Gets the fontsize of the text label
**/
int text_label_get_fontsize(text_label_t *text);

/**
 * Gets the font of the text label
**/
char* text_label_get_font(text_label_t *text);

/**
 * Gets the color of the text label
**/
rgba_color_t text_label_get_color(text_label_t *text);

#endif