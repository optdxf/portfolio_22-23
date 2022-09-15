#include "text_label.h"
#include "vector.h"
#include "color.h"
#include "sdl_wrapper2.h"
#include <stdbool.h>

struct text_label {
    char* message;
    bool visible;
    vector_t position; // must be top-left
    int fontsize;
    char* font; //filepath
    rgba_color_t color;
};

text_label_t* text_label_init(char* message, char *font, vector_t position, int fontsize, bool visible, rgba_color_t color) {
    text_label_t *text = malloc(sizeof(*text));
    text->message = message;
    text->font = font;
    text->position = position;
    text->fontsize = fontsize;
    text->visible = visible;
    text->color = color;
    return text;
}

char* text_label_get_message(text_label_t *text) { return text->message; }

bool text_label_get_visible(text_label_t *text) {
    return text->visible;
}

vector_t text_label_get_position(text_label_t *text) { return text->position; }

int text_label_get_fontsize(text_label_t *text) { return text->fontsize; }

char* text_label_get_font(text_label_t *text) { return text->font; }

rgba_color_t text_label_get_color(text_label_t *text) { return text->color; }