#ifndef __SCENE_H__
#define __SCENE_H__

#include "body.h"
#include "list.h"
#include "ui_group.h"

typedef struct scene scene_t;

typedef void (*force_creator_t)(list_t *bodies, void *aux);

double *scene_get_delta_ptr(scene_t *scene);
void scene_set_ui(scene_t *scene, ui_group_t *ui);
ui_group_t *scene_get_ui(scene_t *scene);
void scene_set_map(scene_t *scene, img_label_t *img);
img_label_t *scene_get_map(scene_t *scene);

scene_t *scene_init();
void scene_free(scene_t *scene);
size_t scene_bodies(scene_t *scene);
body_t *scene_get_body(scene_t *scene, size_t index);
// list_t *scene_get_bodies(scene_t *scene);
void scene_add_body(scene_t *scene, body_t *body);
void scene_add_force_creator(scene_t *scene, force_creator_t forcer, void *aux, free_func_t freer);
void scene_add_bodies_force_creator(scene_t *scene, force_creator_t forcer, void *aux, list_t *bodies, free_func_t freer);
void scene_collide(scene_t *scene, double dt);
void scene_tick(scene_t *scene, double dt);

#endif