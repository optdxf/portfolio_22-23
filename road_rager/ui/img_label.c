#include "img_label.h"
#include "vector.h"
#include "math_util.h"
#include "easing.h"
#include "sdl_wrapper2.h"
#include <stdbool.h>
#include <time.h>


typedef enum tween_status { TWEEN_DEAD, TWEEN_IN_PROGRESS } tween_status_t;

typedef struct tween_data {
    tween_style_t style;
    tween_direction_t direction;
    tween_status_t status;
    vector_t start;
    vector_t end;
    double duration;
    clock_t start_time;
} tween_data_t;


struct img_label {
    bool visible;
    vector_t position;
    double aspect_ratio;    // width:height
    vector_t size;
    anchor_point_t anchor_point;
    const char *fpath;
    SDL_Texture *texture;
    tween_data_t *tween_pos_data;
    tween_data_t *tween_size_data;
    image_mouse_state_t mouse_state;
};

AHFloat interpolate(double start, double end, AHFloat pos, tween_style_t tween_style, tween_direction_t tween_dir) {
    switch (tween_style) {
        case TWEEN_LINEAR: return start + (end - start) * LinearInterpolation(pos);
        case TWEEN_QUADRATIC: {
            switch (tween_dir) {
                case TWEEN_IN: return start + (end - start) * QuadraticEaseIn(pos);
                case TWEEN_OUT: return start + (end - start) * QuadraticEaseOut(pos);
                case TWEEN_INOUT: return start + (end - start) * QuadraticEaseInOut(pos);
            }
        }        
    }
}

void img_label_free(img_label_t *img) {
    if (img->tween_pos_data) free(img->tween_pos_data);
    if (img->tween_size_data) free(img->tween_size_data);
    free(img);
    // do not free sdl_texture; this is cached
}

img_label_t *img_label_init(const char *filepath, image_type_t image_type, vector_t pos, double desired_width, anchor_point_t anchor_point, bool visible) {
    img_label_t *img = malloc(sizeof(*img));
    img->position = pos;
    img->anchor_point = anchor_point;
    img->visible = visible;
    img->tween_pos_data = NULL;
    //img->tween_size_data = NULL;
    img->texture = sdl_get_texture(filepath, image_type);

    int width = 0, height = 0;
    SDL_QueryTexture(img->texture, NULL, NULL, &width, &height);
    img->aspect_ratio = (double)width / height;
    img->size = (vector_t) { desired_width, desired_width / img->aspect_ratio};
    img->mouse_state = MOUSE_STATE_OUTSIDE;

    return img;
}

void img_label_register_pos_tween(img_label_t *img, tween_style_t style, tween_direction_t dir, vector_t start, vector_t end, double duration) {
    if (img->tween_pos_data)
        free(img->tween_pos_data);
    tween_data_t *data = (tween_data_t*)malloc(sizeof(*data));
    data->style = style;
    data->direction = dir;
    data->status = TWEEN_IN_PROGRESS;
    data->start = start;
    data->end = end;
    data->duration = duration;
    data->start_time = clock();

    img->tween_pos_data = data;
}

void img_label_register_size_tween(img_label_t *img, tween_style_t style, tween_direction_t dir, vector_t start, vector_t end, double duration) {
    if (img->tween_size_data)
        free(img->tween_size_data);
    // adjust start/end based on width (so that we maintain aspect ratio; we don't error)
    start.y = start.x / img->aspect_ratio;
    end.y = end.x / img->aspect_ratio;

    tween_data_t *data = (tween_data_t*)malloc(sizeof(*data));
    data->style = style;
    data->direction = dir;
    data->status = TWEEN_IN_PROGRESS;
    data->start = start;
    data->end = end;
    data->duration = duration;
    data->start_time = clock();
    
    img->tween_size_data = data;
}

void img_label_tick(img_label_t *img) {
    if (img->tween_pos_data) {
        tween_data_t *tween_data = img->tween_pos_data;
        if (tween_data->status == TWEEN_DEAD) {
            free(tween_data);
            img->tween_pos_data = NULL;
        } else {
            double delta = (double)(clock() - tween_data->start_time) / CLOCKS_PER_SEC;
            double pos = clamp_dbl(delta / tween_data->duration, 0, 1);
            img->position = (vector_t) {
                interpolate(img->position.x, tween_data->start.x, pos, tween_data->style, tween_data->direction),
                interpolate(img->position.y, tween_data->start.y, pos, tween_data->style, tween_data->direction)
            };
            if (pos == 1) {
                tween_data->status = TWEEN_DEAD;
            }
        }
    }
    
    if (img->tween_size_data) {
        tween_data_t *tween_data = img->tween_size_data;
        if (tween_data->status == TWEEN_DEAD) {
            free(tween_data);
            img->tween_size_data = NULL;
        } else {
            double delta = (double)(clock() - tween_data->start_time) / CLOCKS_PER_SEC;
            double pos = clamp_dbl(delta / tween_data->duration, 0, 1);
            img->size = (vector_t) {
                interpolate(img->size.x, tween_data->start.x, pos, tween_data->style, tween_data->direction),
                interpolate(img->size.y, tween_data->start.y, pos, tween_data->style, tween_data->direction)
            };
            if (pos == 1) {
                tween_data->status = TWEEN_DEAD;
            }
        }
    }
}

bool img_label_is_pos_inside(img_label_t *img, vector_t pos) {
    vector_t absolute_pos = img_label_get_absolute_pos(img);
    return between_dbl(pos.x, absolute_pos.x, absolute_pos.x + img->size.x) &&
        between_dbl(pos.y, absolute_pos.y - img->size.y, absolute_pos.y);
}

void img_label_set_mouse_state(img_label_t *img, image_mouse_state_t state) {
    img->mouse_state = state;
}

image_mouse_state_t img_label_get_mouse_state(img_label_t *img) {
    return img->mouse_state;
}

const char *img_label_get_fpath(img_label_t *img) {
    return img->fpath;
}

SDL_Texture *img_label_get_texture(img_label_t *img) {
    return img->texture;
}

vector_t img_label_get_absolute_pos(img_label_t *img) {
    switch (img->anchor_point) {
        case ANCHOR_TOP_LEFT: return img->position;
        case ANCHOR_CENTER: return (vector_t) { img->position.x - img->size.x / 2, img->position.y + img->size.y / 2 };
        case ANCHOR_TOP_CENTER: return (vector_t) { img->position.x - img->size.x / 2, img->position.y };
    }
}

bool img_label_get_visible(img_label_t *img) {
    return img->visible;
}
void img_label_set_visible(img_label_t *img, bool v) {
    img->visible = v;
}
vector_t img_label_get_position(img_label_t *img) {
    return img->position;
}
void img_label_set_position(img_label_t *img, vector_t pos) {
    img->position = pos;
}
vector_t img_label_get_size(img_label_t *img) {
    return img->size;
}
void img_label_set_size(img_label_t *img, double width) {
    img->size = (vector_t) { width, width / img->aspect_ratio };
}