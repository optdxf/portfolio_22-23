#include "ui_group.h"
#include <stdlib.h>

//----------------

const size_t INITIAL_GUI_LIST_SIZE = 10;
const size_t INITIAL_CALLBACK_LIST_SIZE = 10;

//-----------------

struct ui_group {
    list_t *guis;   // at the moment, this just supports img_labels
    list_t *texts; // supports text_labels only
    list_t *mouse_enter_callbacks;
    list_t *mouse_leave_callbacks;
    list_t *mouse_btn_down_callbacks;
    list_t *mouse_btn_up_callbacks;
};

typedef struct {
    img_label_t *img;
    mouse_callback_func_t callback;
    void *data;
    free_func_t data_freer;
} callback_data_t;

void callback_free(callback_data_t *callback_data) {
    // do not free img; these are independent
    if (callback_data->data && callback_data->data_freer) callback_data->data_freer(callback_data->data);
    free(callback_data);
}

ui_group_t *ui_group_init() {
    ui_group_t *group = (ui_group_t*)malloc(sizeof(*group));
    group->guis = list_init(INITIAL_GUI_LIST_SIZE, (free_func_t)img_label_free);
    group->texts = list_init(INITIAL_GUI_LIST_SIZE, free);
    group->mouse_enter_callbacks = list_init(INITIAL_CALLBACK_LIST_SIZE, (free_func_t)callback_free);
    group->mouse_leave_callbacks = list_init(INITIAL_CALLBACK_LIST_SIZE, (free_func_t)callback_free);
    group->mouse_btn_down_callbacks = list_init(INITIAL_CALLBACK_LIST_SIZE, (free_func_t)callback_free);
    group->mouse_btn_up_callbacks = list_init(INITIAL_CALLBACK_LIST_SIZE, (free_func_t)callback_free);
    return group;
}

void ui_group_add(ui_group_t *group, img_label_t *img) {
    list_add(group->guis, img);
}

void ui_group_add_text(ui_group_t *group, text_label_t *text) {
    list_add(group->texts, text);
}

void ui_group_clear_texts(ui_group_t *group) {
    list_free(group->texts);
    group->texts = list_init(1, free);
}

list_t *ui_group_get_texts(ui_group_t *group) {
    return group->texts;
}

void ui_group_set_callback(ui_group_t *group, img_label_t *img, callback_type_t type, mouse_callback_func_t func, void *data, free_func_t data_freer) {
    callback_data_t *callback_data = (callback_data_t*)malloc(sizeof(*callback_data));
    callback_data->img = img;
    callback_data->callback = func;
    callback_data->data = data;
    callback_data->data_freer = data_freer;
    switch (type) {
        case CALLBACK_MOUSE_ENTER: return list_add(group->mouse_enter_callbacks, callback_data);
        case CALLBACK_MOUSE_LEAVE: return list_add(group->mouse_leave_callbacks, callback_data);
        case CALLBACK_MOUSE_DOWN: return list_add(group->mouse_btn_down_callbacks, callback_data);
        case CALLBACK_MOUSE_UP: return list_add(group->mouse_btn_up_callbacks, callback_data);
    }
}

void ui_group_set_texts(ui_group_t *group, list_t *texts) {
    group->texts = texts;
}

list_t *ui_group_get_guis(ui_group_t *group) {
    return group->guis; // returns direct ptr
}

list_t *ui_group_get_callbacks(ui_group_t *group, callback_type_t type) {
    switch (type) {
        case CALLBACK_MOUSE_ENTER: return group->mouse_enter_callbacks;
        case CALLBACK_MOUSE_LEAVE: return group->mouse_leave_callbacks;
        case CALLBACK_MOUSE_DOWN: return group->mouse_btn_down_callbacks;
        case CALLBACK_MOUSE_UP:  return group->mouse_btn_up_callbacks;
    }
}

void ui_group_process_mouse_event(ui_group_t *group, double x, double y, mouse_event_type_t type) {
    vector_t pos = (vector_t) { x, y };
   // printf("called\n");

    switch (type) {
        case EVENT_MOUSE_MOVE: {
            for (size_t i = 0; i < list_size(group->mouse_enter_callbacks); ++i) {
                callback_data_t *callback_data = list_get(group->mouse_enter_callbacks, i);
                img_label_t *img = callback_data->img;
                if (img_label_get_mouse_state(img) == MOUSE_STATE_OUTSIDE && img_label_is_pos_inside(img, pos)) {
                    // has entered now now
                    img_label_set_mouse_state(img, MOUSE_STATE_INSIDE);
                    callback_data->callback(img, callback_data->data);
                }
            }
            for (size_t i = 0; i < list_size(group->mouse_leave_callbacks); ++i) {
                callback_data_t *callback_data = list_get(group->mouse_leave_callbacks, i);
                img_label_t *img = callback_data->img;
                if (img_label_get_mouse_state(img) == MOUSE_STATE_INSIDE && !img_label_is_pos_inside(img, pos)) {
                    // has entered now now
                    img_label_set_mouse_state(img, MOUSE_STATE_OUTSIDE);
                    callback_data->callback(img, callback_data->data);
                }
            }
            break;
        }
        case EVENT_MOUSE_PRESSED: {
            //printf("pressed!\n");
            for (size_t i = 0; i < list_size(group->mouse_btn_down_callbacks); ++i) {
                callback_data_t *callback_data = list_get(group->mouse_btn_down_callbacks, i);
                img_label_t *img = callback_data->img;
                if (img_label_is_pos_inside(img, pos)) {
                    callback_data->callback(img, callback_data->data);
                }
            }
            break;
        }
        case EVENT_MOUSE_RELEASED: {
            //printf("released\n");
            for (size_t i = 0; i < list_size(group->mouse_btn_up_callbacks); ++i) {
                callback_data_t *callback_data = list_get(group->mouse_btn_up_callbacks, i);
                img_label_t *img = callback_data->img;
                if (img_label_is_pos_inside(img, pos)) {
                    callback_data->callback(img, callback_data->data);
                }
            }
            break;
        }
    }
}