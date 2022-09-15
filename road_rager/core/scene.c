#include "scene.h"
#include "list.h"
#include "ui_group.h"
#include "img_label.h"
#include <assert.h>

typedef struct force_creator_data {
	force_creator_t creator;
	void *data;
	list_t *bodies;
	free_func_t freer;
} force_creator_data_t;

struct scene {
    list_t *bodies;
    ui_group_t *ui; // scene does not own ui_group, nor map_img
    img_label_t *map_img;
    list_t *force_creators;
	double *delta;
};

const size_t DEFAULT_BODY_LIST_SIZE = 25;
const size_t DEFAULT_FORCES_LIST_SIZE = 25;

double *scene_get_delta_ptr(scene_t *scene) { return scene->delta; }

void force_creator_data_free(force_creator_data_t *fcreator) {
	if (fcreator->freer) {
		fcreator->freer(fcreator->data);
	}
	if (fcreator->bodies) {
		list_free(fcreator->bodies);
	}
	free(fcreator);
}

scene_t *scene_init() {
    scene_t *scene = (scene_t*)malloc(sizeof(*scene));
    assert("could not allocate memory" && scene != NULL);

    scene->bodies = list_init(DEFAULT_BODY_LIST_SIZE, (free_func_t)body_free);
    scene->force_creators = list_init(DEFAULT_FORCES_LIST_SIZE, (free_func_t)force_creator_data_free);
    scene->ui = NULL;
    scene->map_img = NULL;
    scene->delta = malloc(sizeof(double));
	*scene->delta = 0;
    return scene;
}

void scene_free(scene_t *scene) {
	list_free(scene->bodies);
	list_free(scene->force_creators);
	free(scene->delta);
	free(scene);
}

size_t scene_bodies(scene_t *scene) { return list_size(scene->bodies); }

body_t *scene_get_body(scene_t *scene, size_t index) {
	assert("index out of bounds" && index < scene_bodies(scene));
	return (body_t *)list_get(scene->bodies, index);
}

void scene_add_body(scene_t *scene, body_t *body) { list_add(scene->bodies, body); }

void scene_remove_body(scene_t *scene, size_t index) {
	assert("index out of bounds" && index < scene_bodies(scene));
	body_remove((body_t *)list_get(scene->bodies, index));
}

void scene_add_force_creator(
	scene_t *scene, force_creator_t forcer, void *aux, free_func_t freer) {
	force_creator_data_t *fcreator = malloc(sizeof(force_creator_data_t));
	fcreator->creator = forcer;
	fcreator->data = aux;
	fcreator->freer = freer;
	fcreator->bodies = NULL;
	list_add(scene->force_creators, fcreator);
}

void scene_add_bodies_force_creator(scene_t *scene, force_creator_t forcer, void *aux,
	list_t *bodies, free_func_t freer) {
	force_creator_data_t *fcreator = malloc(sizeof(force_creator_data_t));
	fcreator->creator = forcer;
	fcreator->data = aux;
	fcreator->freer = freer;
	fcreator->bodies = bodies;
	list_add(scene->force_creators, fcreator);
}

void scene_collide(scene_t *scene, double dt) {
	*scene->delta = dt;
	for (size_t i = 0; i < list_size(scene->force_creators); i++) {
		force_creator_data_t *fcreator =
			(force_creator_data_t *)list_get(scene->force_creators, i);
		fcreator->creator(fcreator->bodies, fcreator->data);
	}
}

void scene_tick(scene_t *scene, double dt) {
	*scene->delta = dt;

	for (size_t i = 0; i < list_size(scene->bodies); i++) {
		body_tick(list_get(scene->bodies, i), dt);
	}

	// after executing ticks, check if any should be removed
	for (size_t i = 0; i < list_size(scene->force_creators); i++) {
		force_creator_data_t *fcreator =
			(force_creator_data_t *)list_get(scene->force_creators, i);
		if (fcreator->bodies) {
			for (size_t j = 0; j < list_size(fcreator->bodies); j++) {
				if (body_is_removed((body_t *)list_get(fcreator->bodies, j))) {
					force_creator_data_free(list_remove(scene->force_creators, i--));
					break;
				}
			}
		}
	}

	for (size_t i = 0; i < list_size(scene->bodies); i++) {
		if (body_is_removed((body_t *)list_get(scene->bodies, i))) {
			body_free(list_remove(scene->bodies, i--));
		}
	}
}

img_label_t *scene_get_map(scene_t *scene) {
    return scene->map_img;
}

void scene_set_map(scene_t *scene, img_label_t *img) {
    scene->map_img = img;
}

ui_group_t *scene_get_ui(scene_t *scene) {
    return scene->ui;
}

void scene_set_ui(scene_t *scene, ui_group_t *ui) {
    scene->ui = ui;
}