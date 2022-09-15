#ifndef __UI_GROUP__
#define __UI_GROUP__

#include "img_label.h"
#include "text_label.h"
#include "list.h"
#include "sdl_constants.h"

/**
* Collection of UI objects (images/text); includes callbacks for the UI elements
*/
typedef struct ui_group ui_group_t;

/**
* Signature of callback functions
*/
typedef void (*mouse_callback_func_t)(img_label_t *img, void *data);

/**
*  Callback types
*/
typedef enum callback_type { CALLBACK_MOUSE_ENTER, CALLBACK_MOUSE_LEAVE, CALLBACK_MOUSE_DOWN, CALLBACK_MOUSE_UP } callback_type_t;

/**
* Signature of callback functions
*/
ui_group_t *ui_group_init();

/**
* Add an image label to a group
*/
void ui_group_add(ui_group_t *group, img_label_t *img);

/**
* Add a text lable to a group
*/
void ui_group_add_text(ui_group_t *group, text_label_t *text);

/**
* Set a callback for a given image; callback is maintained by the group creator.
*/
void ui_group_set_callback(ui_group_t *group, img_label_t *img, callback_type_t type, mouse_callback_func_t func, void *data, free_func_t data_freer);

/**
* Returns a pointer to a ui_group
*/
list_t *ui_group_get_guis(ui_group_t *group);

/**
* Get texts of ui group.
*/
list_t *ui_group_get_texts(ui_group_t *group);

void ui_group_clear_texts(ui_group_t *group);

/**
* Aets test of UI group
*/
void ui_group_set_texts(ui_group_t *group, list_t *texts);

/**
* Gets all callbacks of a certain type from a group
*/
list_t *ui_group_get_callbacks(ui_group_t *group, callback_type_t type);

/**
* Applies mouse events to callback functions
*/
void ui_group_process_mouse_event(ui_group_t *group, double x, double y, mouse_event_type_t type);

#endif