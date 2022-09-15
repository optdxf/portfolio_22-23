#ifndef __IMG_LABEL_H__
#define __IMG_LABEL_H__

#include <SDL2/SDL.h>
#include "vector.h"

/**
 * img_label.h is an abstraction layer over images; it is a wrapper class that blends in
 * other properties like position, size, texture, etc.
 * 
 */

typedef enum tween_style { TWEEN_LINEAR, TWEEN_QUADRATIC } tween_style_t;
typedef enum tween_direction { TWEEN_IN, TWEEN_OUT, TWEEN_INOUT } tween_direction_t;
typedef enum anchor_point { ANCHOR_CENTER, ANCHOR_TOP_LEFT, ANCHOR_TOP_CENTER } anchor_point_t;
typedef enum image_type { IMG_TYPE_PNG, IMG_TYPE_SVG } image_type_t;
typedef enum image_mouse_state { MOUSE_STATE_INSIDE, MOUSE_STATE_OUTSIDE } image_mouse_state_t;

typedef struct img_label img_label_t;

void img_label_free(img_label_t *img);
img_label_t *img_label_init(const char *filepath, image_type_t image_type, vector_t pos, double width, anchor_point_t anchor_point, bool visible);
void img_label_register_pos_tween(img_label_t *img, tween_style_t style, tween_direction_t dir, vector_t start, vector_t end, double duration);
void img_label_register_size_tween(img_label_t *img, tween_style_t style, tween_direction_t dir, vector_t start, vector_t end, double duration);
void img_label_tick(img_label_t *img);
vector_t img_label_get_absolute_pos(img_label_t *img);
bool img_label_is_pos_inside(img_label_t *img, vector_t pos);
image_mouse_state_t img_label_get_mouse_state(img_label_t *img);
void img_label_set_mouse_state(img_label_t *img, image_mouse_state_t state);
const char *img_label_get_fpath(img_label_t *img);
SDL_Texture *img_label_get_texture(img_label_t *img);
bool img_label_get_visible(img_label_t *img);
void img_label_set_visible(img_label_t *img, bool v);
vector_t img_label_get_position(img_label_t *img);
void img_label_set_position(img_label_t *img, vector_t pos);
vector_t img_label_get_size(img_label_t *img);
void img_label_set_size(img_label_t *img, double width);

#endif