#include "ui_handler.h"
#include "global_constants.h"
#include "client_launcher.h"
#include "text_label.h"
#include "server.h"
#include <stdio.h>

// We use global variables b/c there will always be just 1 UI handler (just 1 GUI type)
ui_group_t *main_menu = NULL;
ui_group_t *single_player_car_selection = NULL;
ui_group_t *multi_player_car_selection = NULL;
ui_group_t *single_player_map_selection = NULL;
ui_group_t *single_player_instructions = NULL;
ui_group_t *multi_player_instructions = NULL;
ui_group_t *single_player_game = NULL;
ui_group_t *multiplayer_game = NULL;
ui_group_t *multi_player_lobby = NULL;
ui_group_t *single_player_game_over = NULL;
ui_group_t *multi_player_game_over = NULL;

// UI positioning in game
char* font = "assets/fonts/GROCHES.ttf";
int lobby_fontsize = 150;
vector_t first_lobby_position = (vector_t) { 4096 / 2.0 - 125, 2048 / 2.0 + 350 };
vector_t lobby_pos_increment = (vector_t) { 0.0, -175.0 };

vector_t lap_counter_position = (vector_t) { 4096.0 / 2 + 375, 2048 - 40 };
vector_t multiplayer_lap_counter_position = (vector_t) { 4096.0 / 2 - 75, 2048 - 40 };
vector_t timer_position = (vector_t) { 4096.0 / 2 - 525, 2048 - 40 };

int game_over_fontsize = 500;
int multi_player_game_over_fontsize = 200;
vector_t singleplayer_game_over_text_position = (vector_t) { 4096.0 / 2 - 450, 2048 / 2.0 - 150 };
vector_t multiplayer_game_over_text_position = (vector_t) { 4096.0 / 2 - 450, 2048 / 2.0 - 150 };


// typedef enum { MAIN_MENU } ui_group_type_t;

state_t *state = NULL;
list_t *car_list_splayer = NULL;
list_t *car_list_mplayer = NULL;
list_t *map_list = NULL;
size_t current_car_splayer = 0;
size_t current_car_mplayer = 0;
size_t current_map = 0;

// Main menu
const char *LOGO_FPATH = "assets/menu/logo.png";
const image_type_t LOGO_TYPE = IMG_TYPE_PNG;
const vector_t LOGO_POS = (vector_t) { 4096 / 2.0, 2048.0 - 100 };
const double LOGO_WIDTH = 1850;
const anchor_point_t LOGO_ANCHOR_POINT = ANCHOR_TOP_CENTER;

const char *SPLAYER_FPATH = "assets/menu/single_plr_btn.png";
const image_type_t SPLAYER_TYPE = IMG_TYPE_PNG;
const vector_t SPLAYER_POS = (vector_t) { 4096 / 2.0 - 500, 550.0 };
const double SPLAYER_WIDTH = 500;
const anchor_point_t SPLAYER_ANCHOR_POINT = ANCHOR_CENTER;

const char *MPLAYER_FPATH = "assets/menu/multi_plr_btn.png";
const image_type_t MPLAYER_TYPE = IMG_TYPE_PNG;
const vector_t MPLAYER_POS = (vector_t) { 4096 / 2.0 + 500, 550.0 };
const double MPLAYER_WIDTH = 500;
const anchor_point_t MPLAYER_ANCHOR_POINT = ANCHOR_CENTER;

// Single player menu
const char *SPLAYER_INSTRUCTIONS_FPATH = "assets/menu/single_plr_instructions.png"; // change to instructions
const image_type_t SPLAYER_INSTRUCTIONS_TYPE = IMG_TYPE_PNG;
const vector_t SPLAYER_INSTRUCTIONS_POS = (vector_t) { 4096 / 2.0, 2048 / 2.0 };
const double SPLAYER_INSTRUCTIONS_WIDTH = 1500;
const anchor_point_t SPLAYER_INSTRUCTIONS_ANCHOR_POINT = ANCHOR_CENTER;

const char *MAP1_FPATH = "assets/menu/oval_preview.png";
const image_type_t MAP1_TYPE = IMG_TYPE_PNG;
const char *MAP2_FPATH = "assets/menu/italy_preview.png";
const image_type_t MAP2_TYPE = IMG_TYPE_PNG;
const vector_t MAP_POS = (vector_t) { 4096 / 2.0, 2048 / 2.0 - 60};
const double MAP_DISPLAY_WIDTH = 1650;
const anchor_point_t MAP_ANCHOR_POINT = ANCHOR_CENTER;

const char *SELECT_MAP_FPATH = "assets/menu/select_map.png";
const image_type_t SELECT_MAP_TYPE = IMG_TYPE_PNG;
const vector_t SELECT_MAP_POS = (vector_t) { 4096 / 2.0, 2048 / 2.0 };
const double SELECT_MAP_WIDTH = 2500;
const anchor_point_t SELECT_MAP_ANCHOR_POINT = ANCHOR_CENTER;

const char *MAP_RIGHT_ARROW_FPATH = "assets/menu/right_btn.png";
const image_type_t MAP_RIGHT_ARROW_TYPE = IMG_TYPE_PNG;
const vector_t MAP_RIGHT_ARROW_POS = (vector_t) { 4096 / 2.0 + 1200, 2048 / 2.0 };
const double MAP_RIGHT_ARROW_WIDTH = 200;
const anchor_point_t MAP_RIGHT_ARROW_ANCHOR_POINT = ANCHOR_CENTER;

const char *MAP_LEFT_ARROW_FPATH = "assets/menu/left_btn.png";
const image_type_t MAP_LEFT_ARROW_TYPE = IMG_TYPE_PNG;
const vector_t MAP_LEFT_ARROW_POS = (vector_t) { 4096 / 2.0 - 1200, 2048 / 2.0 };
const double MAP_LEFT_ARROW_WIDTH = 200;
const anchor_point_t MAP_LEFT_ARROW_ANCHOR_POINT = ANCHOR_CENTER;

const char *MAP_SELECT_CHECKMARK_FPATH = "assets/menu/checkmark.png";
const image_type_t MAP_SELECT_CHECKMARK_TYPE = IMG_TYPE_PNG;
const vector_t MAP_SELECT_CHECKMARK_POS = (vector_t) { 4096 / 2.0, 2048 / 2.0 - 825 };
const double MAP_SELECT_CHECKMARK_WIDTH = 300;
const anchor_point_t MAP_SELECT_CHECKMARK_ANCHOR_POINT = ANCHOR_CENTER;

const char *SPLAYER_INSTRUCTIONS_NEXT_FPATH = "assets/menu/checkmark.png";
const image_type_t SPLAYER_INSTRUCTIONS_NEXT_TYPE = IMG_TYPE_PNG;
const vector_t SPLAYER_INSTRUCTIONS_NEXT_POS = (vector_t) { 4096 / 2.0 + 450, 2048 / 2.0 - 600 };
const double SPLAYER_INSTRUCTIONS_NEXT_WIDTH = 300;
const anchor_point_t SPLAYER_INSTRUCTIONS_NEXT_ANCHOR_POINT = ANCHOR_CENTER;

const char *SPLAYER_INSTRUCTIONS_PREV_FPATH = "assets/menu/exit.png";
const image_type_t SPLAYER_INSTRUCTIONS_PREV_TYPE = IMG_TYPE_PNG;
const vector_t SPLAYER_INSTRUCTIONS_PREV_POS = (vector_t) { 4096 / 2.0 - 450, 2048 / 2.0 - 600 };
const double SPLAYER_INSTRUCTIONS_PREV_WIDTH = 300;
const anchor_point_t SPLAYER_INSTRUCTIONS_PREV_ANCHOR_POINT = ANCHOR_CENTER;

// Multiplayer menu
const char *MPLAYER_INSTRUCTIONS_FPATH = "assets/menu/multi_plr_instructions.png";
const image_type_t MPLAYER_INSTRUCTIONS_TYPE = IMG_TYPE_PNG;
const vector_t MPLAYER_INSTRUCTIONS_POS = (vector_t) { 4096 / 2.0, 2048 / 2.0 };
const double MPLAYER_INSTRUCTIONS_WIDTH = 1500;
const anchor_point_t MPLAYER_INSTRUCTIONS_ANCHOR_POINT = ANCHOR_CENTER;

const char *MPLAYER_INSTRUCTIONS_NEXT_FPATH = "assets/menu/checkmark.png";
const image_type_t MPLAYER_INSTRUCTIONS_NEXT_TYPE = IMG_TYPE_PNG;
const vector_t MPLAYER_INSTRUCTIONS_NEXT_POS = (vector_t) { 4096 / 2.0 + 450, 2048 / 2.0 - 600 };
const double MPLAYER_INSTRUCTIONS_NEXT_WIDTH = 300;
const anchor_point_t MPLAYER_INSTRUCTIONS_NEXT_ANCHOR_POINT = ANCHOR_CENTER;

const char *MPLAYER_INSTRUCTIONS_PREV_FPATH = "assets/menu/exit.png";
const image_type_t MPLAYER_INSTRUCTIONS_PREV_TYPE = IMG_TYPE_PNG;
const vector_t MPLAYER_INSTRUCTIONS_PREV_POS = (vector_t) { 4096 / 2.0 - 450, 2048 / 2.0 - 600 };
const double MPLAYER_INSTRUCTIONS_PREV_WIDTH = 300;
const anchor_point_t MPLAYER_INSTRUCTIONS_PREV_ANCHOR_POINT = ANCHOR_CENTER;

const char *MPLAYER_LOBBY_FPATH = "assets/menu/lobby.png";
const image_type_t MPLAYER_LOBBY_TYPE = IMG_TYPE_PNG;
const vector_t MPLAYER_LOBBY_POS = (vector_t) { 4096 / 2.0, 2048 / 2.0 };
const double MPLAYER_LOBBY_WIDTH = 1500;
const anchor_point_t MPLAYER_LOBBY_ANCHOR_POINT = ANCHOR_CENTER;

// Shared menu
const char *SELECT_CAR_FPATH = "assets/menu/select_car.png";
const image_type_t SELECT_CAR_TYPE = IMG_TYPE_PNG;
const vector_t SELECT_CAR_POS = (vector_t) { 4096 / 2.0, 2048 / 2.0 };
const double SELECT_CAR_WIDTH = 1500;
const anchor_point_t SELECT_CAR_ANCHOR_POINT = ANCHOR_CENTER;

const char *RIGHT_ARROW_FPATH = "assets/menu/right_btn.png";
const image_type_t RIGHT_ARROW_TYPE = IMG_TYPE_PNG;
const vector_t RIGHT_ARROW_POS = (vector_t) { 4096 / 2.0 + 600, 2048 / 2.0 };
const double RIGHT_ARROW_WIDTH = 200;
const anchor_point_t RIGHT_ARROW_ANCHOR_POINT = ANCHOR_CENTER;

const char *LEFT_ARROW_FPATH = "assets/menu/left_btn.png";
const image_type_t LEFT_ARROW_TYPE = IMG_TYPE_PNG;
const vector_t LEFT_ARROW_POS = (vector_t) { 4096 / 2.0 - 550, 2048 / 2.0 };
const double LEFT_ARROW_WIDTH = 200;
const anchor_point_t LEFT_ARROW_ANCHOR_POINT = ANCHOR_CENTER;

const char *CAR_SELECT_CHECKMARK_FPATH = "assets/menu/checkmark.png";
const image_type_t CAR_SELECT_CHECKMARK_TYPE = IMG_TYPE_PNG;
const vector_t CAR_SELECT_CHECKMARK_POS = (vector_t) { 4096 / 2.0, 2048 / 2.0 - 550 };
const double CAR_SELECT_CHECKMARK_WIDTH = 300;
const anchor_point_t CAR_SELECT_CHECKMARK_ANCHOR_POINT = ANCHOR_CENTER;

const char *CAR1_FPATH = "assets/menu/audi_selection.png";
const image_type_t CAR1_TYPE = IMG_TYPE_PNG;
const char *CAR2_FPATH = "assets/menu/black_viper_selection.png";
const image_type_t CAR2_TYPE = IMG_TYPE_PNG;
const vector_t CAR1_POS = (vector_t) { 4096 / 2.0, 2048 / 2.0 - 35 };
const vector_t CAR2_POS = (vector_t) { 4096 / 2.0, 2048 / 2.0 -35};
const double CAR1_DISPLAY_WIDTH = 400;
const double CAR2_DISPLAY_WIDTH = 450;
const anchor_point_t CAR_ANCHOR_POINT = ANCHOR_CENTER;


// Single player in game UI
const char *SP_LAP_COUNTER_FPATH = "assets/menu/lap_counter.png";
const image_type_t SP_LAP_COUNTER_TYPE = IMG_TYPE_PNG;
const vector_t SP_LAP_COUNTER_POS = (vector_t) { 4096.0 / 2 + 400, 2048 - 100 };
const double SP_LAP_COUNTER_WIDTH = 600;
const anchor_point_t SP_LAP_COUNTER_ANCHOR_POINT = ANCHOR_CENTER;

const char *SP_TIMER_FPATH = "assets/menu/time_left.png";
const image_type_t SP_TIMER_TYPE = IMG_TYPE_PNG;
const vector_t SP_TIMER_POS = (vector_t) { 4096.0 / 2 - 400, 2048 - 100 };
const double SP_TIMER_WIDTH = 600;
const anchor_point_t SP_TIMER_ANCHOR_POINT = ANCHOR_CENTER;

const char *SP_PAUSE_FPATH = "assets/menu/pause_btn.png";
const image_type_t SP_PAUSE_TYPE = IMG_TYPE_PNG;
const vector_t SP_PAUSE_POS = (vector_t) { 4096 - 300, 2048 - 100 };
const double SP_PAUSE_WIDTH = 200;
const anchor_point_t SP_PAUSE_ANCHOR_POINT = ANCHOR_CENTER;

const char *CONTROLS_FPATH = "assets/menu/no_background_controls.png";
const image_type_t CONTROLS_TYPE = IMG_TYPE_PNG;
const vector_t CONTROLS_POS = (vector_t) { 300, 200 };
const double CONTROLS_WIDTH = 400;
const anchor_point_t CONTROLS_ANCHOR_POINT = ANCHOR_CENTER;

// Multiplayer in game UI
const char *MP_LAP_COUNTER_FPATH = "assets/menu/lap_counter.png";
const image_type_t MP_LAP_COUNTER_TYPE = IMG_TYPE_PNG;
const vector_t MP_LAP_COUNTER_POS = (vector_t) { 4096.0 / 2, 2048 - 100 };
const double MP_LAP_COUNTER_WIDTH = 600;
const anchor_point_t MP_LAP_COUNTER_ANCHOR_POINT = ANCHOR_CENTER;

// Game over screen
const char *SPLAYER_GAME_OVER_FPATH = "assets/menu/singleplayer_game_over.png";
const image_type_t SPLAYER_GAME_OVER_TYPE = IMG_TYPE_PNG;
const vector_t SPLAYER_GAME_OVER_POS = (vector_t) { 4096 / 2.0, 2048 / 2.0 };
const double SPLAYER_GAME_OVER_WIDTH = 2000;
const anchor_point_t SPLAYER_GAME_OVER_ANCHOR_POINT = ANCHOR_CENTER;

const char *MPLAYER_GAME_OVER_FPATH = "assets/menu/singleplayer_game_over.png";
const image_type_t MPLAYER_GAME_OVER_TYPE = IMG_TYPE_PNG;
const vector_t MPLAYER_GAME_OVER_POS = (vector_t) { 4096 / 2.0, 2048 / 2.0 };
const double MPLAYER_GAME_OVER_WIDTH = 2000;
const anchor_point_t MPLAYER_GAME_OVER_ANCHOR_POINT = ANCHOR_CENTER;


void splayer_selected(img_label_t *button, void *data) {
    scene_set_ui(state->scene, single_player_instructions);
}

void splayer_instructions_next(img_label_t *button, void *data) {
    state->mode = SINGLE;
    scene_set_ui(state->scene, single_player_car_selection);
}

void splayer_instructions_back(img_label_t *button, void *data) {
    scene_set_ui(state->scene, main_menu);
}

void mplayer_instructions_next(img_label_t *button, void *data) {
    state->mode = MULTI;
    scene_set_ui(state->scene, multi_player_car_selection);
}

void mplayer_instructions_back(img_label_t *button, void *data) {
    scene_set_ui(state->scene, main_menu);
}

void mplayer_selected(img_label_t *button, void *data) {
    scene_set_ui(state->scene, multi_player_instructions);
}

void splayer_map1_selected(img_label_t *map1, void *data) {
    printf("map1\n");
}

void splayer_map2_selected(img_label_t *map2, void *data) {
    printf("map2\n");
}

void right_arrow_car_splayer(img_label_t *button, void *data) {
    img_label_set_visible(list_get(car_list_splayer, current_car_splayer), false);
    current_car_splayer = (current_car_splayer + 1) % list_size(car_list_splayer);
    img_label_set_visible(list_get(car_list_splayer, current_car_splayer), true);
}

void left_arrow_car_splayer(img_label_t *button, void *data) {
    img_label_set_visible(list_get(car_list_splayer, current_car_splayer), false);
    if (current_car_splayer == 0) {
        current_car_splayer = list_size(car_list_splayer) - 1;
    }
    else {
        current_car_splayer--;
    }
    img_label_set_visible(list_get(car_list_splayer, current_car_splayer), true);
}

void right_arrow_car_mplayer(img_label_t *button, void *data) {
    img_label_set_visible(list_get(car_list_mplayer, current_car_mplayer), false);
    current_car_mplayer = (current_car_mplayer + 1) % list_size(car_list_mplayer);
    img_label_set_visible(list_get(car_list_mplayer, current_car_mplayer), true);
}

void left_arrow_car_mplayer(img_label_t *button, void *data) {
    img_label_set_visible(list_get(car_list_mplayer, current_car_mplayer), false);
    if (current_car_mplayer == 0) {
        current_car_mplayer = list_size(car_list_mplayer) - 1;
    }
    else {
        current_car_mplayer--;
    }
    img_label_set_visible(list_get(car_list_mplayer, current_car_mplayer), true);
}

void splayer_car_selected(img_label_t *button, void *data) {
    state->car_choice = current_car_splayer;
    scene_set_ui(state->scene, single_player_map_selection);
}

void right_arrow_map(img_label_t *button, void *data) {
    img_label_set_visible(list_get(map_list, current_map), false);
    current_map = (current_map + 1) % list_size(map_list);
    img_label_set_visible(list_get(map_list, current_map), true);
}

void left_arrow_map(img_label_t *button, void *data) {
    img_label_set_visible(list_get(map_list, current_map), false);
    if (current_map == 0) {
        current_map = list_size(map_list) - 1;
    }
    else {
        current_map--;
    }
    img_label_set_visible(list_get(map_list, current_map), true);
}

void splayer_map_selected(img_label_t *button, void *data) {
    state->map_choice = current_map;
    state->status = STATUS_BEGIN_SINGLE_PLAYER;
    scene_set_ui(state->scene, single_player_game);
}

void mplayer_car_selected(img_label_t *button, void *data) {
    state->car_choice = current_car_mplayer;
    state->status = STATUS_BEGIN_MULTI_PLAYER;
    scene_set_ui(state->scene, multi_player_lobby);
}

img_label_t *map;
img_label_t *logo;

void make_main_menu() {
    main_menu = ui_group_init();
    map = img_label_init("assets/maps/map1.png", IMG_TYPE_PNG, (vector_t){0, SCENE_HEIGHT}, SCENE_WIDTH, ANCHOR_TOP_LEFT, true);
    scene_set_map(state->scene, map);

    logo = img_label_init(LOGO_FPATH, LOGO_TYPE, LOGO_POS, LOGO_WIDTH, LOGO_ANCHOR_POINT, true);
    img_label_t *singleplayer = img_label_init(SPLAYER_FPATH, SPLAYER_TYPE, SPLAYER_POS, SPLAYER_WIDTH, SPLAYER_ANCHOR_POINT, true);
    img_label_t *multiplayer = img_label_init(MPLAYER_FPATH, MPLAYER_TYPE, MPLAYER_POS, MPLAYER_WIDTH, MPLAYER_ANCHOR_POINT, true);
    ui_group_add(main_menu, logo);
    ui_group_add(main_menu, singleplayer);
    ui_group_add(main_menu, multiplayer);

    ui_group_set_callback(main_menu, singleplayer, CALLBACK_MOUSE_UP, splayer_selected, NULL, NULL);
    ui_group_set_callback(main_menu, multiplayer, CALLBACK_MOUSE_UP, mplayer_selected, NULL, NULL);
}

void make_splayer_instructions() {
    single_player_instructions = ui_group_init();
    img_label_t *instructions = img_label_init(SPLAYER_INSTRUCTIONS_FPATH, SPLAYER_INSTRUCTIONS_TYPE, SPLAYER_INSTRUCTIONS_POS, SPLAYER_INSTRUCTIONS_WIDTH, SPLAYER_INSTRUCTIONS_ANCHOR_POINT, true);
    img_label_t *next = img_label_init(SPLAYER_INSTRUCTIONS_NEXT_FPATH, SPLAYER_INSTRUCTIONS_NEXT_TYPE, SPLAYER_INSTRUCTIONS_NEXT_POS, SPLAYER_INSTRUCTIONS_NEXT_WIDTH, SPLAYER_INSTRUCTIONS_NEXT_ANCHOR_POINT, true);
    img_label_t *back = img_label_init(SPLAYER_INSTRUCTIONS_PREV_FPATH, SPLAYER_INSTRUCTIONS_PREV_TYPE, SPLAYER_INSTRUCTIONS_PREV_POS, SPLAYER_INSTRUCTIONS_PREV_WIDTH, SPLAYER_INSTRUCTIONS_PREV_ANCHOR_POINT, true);
        
    ui_group_add(single_player_instructions, instructions);
    ui_group_add(single_player_instructions, next);
    ui_group_add(single_player_instructions, back);

    ui_group_set_callback(single_player_instructions, next, CALLBACK_MOUSE_UP, splayer_instructions_next, NULL, NULL);
    ui_group_set_callback(single_player_instructions, back, CALLBACK_MOUSE_UP, splayer_instructions_back, NULL, NULL);
}

void make_mplayer_instructions() {
    multi_player_instructions = ui_group_init();
    img_label_t *instructions = img_label_init(MPLAYER_INSTRUCTIONS_FPATH, MPLAYER_INSTRUCTIONS_TYPE, MPLAYER_INSTRUCTIONS_POS, MPLAYER_INSTRUCTIONS_WIDTH, MPLAYER_INSTRUCTIONS_ANCHOR_POINT, true);
    img_label_t *next = img_label_init(MPLAYER_INSTRUCTIONS_NEXT_FPATH, MPLAYER_INSTRUCTIONS_NEXT_TYPE, MPLAYER_INSTRUCTIONS_NEXT_POS, MPLAYER_INSTRUCTIONS_NEXT_WIDTH, MPLAYER_INSTRUCTIONS_NEXT_ANCHOR_POINT, true);
    img_label_t *back = img_label_init(MPLAYER_INSTRUCTIONS_PREV_FPATH, MPLAYER_INSTRUCTIONS_PREV_TYPE, MPLAYER_INSTRUCTIONS_PREV_POS, MPLAYER_INSTRUCTIONS_PREV_WIDTH, MPLAYER_INSTRUCTIONS_PREV_ANCHOR_POINT, true);
        
    ui_group_add(multi_player_instructions, instructions);
    ui_group_add(multi_player_instructions, next);
    ui_group_add(multi_player_instructions, back);

    ui_group_set_callback(multi_player_instructions, next, CALLBACK_MOUSE_UP, mplayer_instructions_next, NULL, NULL);
    ui_group_set_callback(multi_player_instructions, back, CALLBACK_MOUSE_UP, mplayer_instructions_back, NULL, NULL);
}

void make_splayer_car_selection() {
    single_player_car_selection = ui_group_init();
    img_label_t *selection_background = img_label_init(SELECT_CAR_FPATH, SELECT_CAR_TYPE, SELECT_CAR_POS, SELECT_CAR_WIDTH, SELECT_CAR_ANCHOR_POINT, true);
    img_label_t *right_arrow = img_label_init(RIGHT_ARROW_FPATH, RIGHT_ARROW_TYPE, RIGHT_ARROW_POS, RIGHT_ARROW_WIDTH, RIGHT_ARROW_ANCHOR_POINT, true);
    img_label_t *left_arrow = img_label_init(LEFT_ARROW_FPATH, LEFT_ARROW_TYPE, LEFT_ARROW_POS, LEFT_ARROW_WIDTH, LEFT_ARROW_ANCHOR_POINT, true);
    img_label_t *checkmark = img_label_init(CAR_SELECT_CHECKMARK_FPATH, CAR_SELECT_CHECKMARK_TYPE, CAR_SELECT_CHECKMARK_POS, CAR_SELECT_CHECKMARK_WIDTH, CAR_SELECT_CHECKMARK_ANCHOR_POINT, true);

    car_list_splayer = list_init(2, (free_func_t)img_label_free);
    img_label_t *car1 = img_label_init(CAR1_FPATH, CAR1_TYPE, CAR1_POS, CAR1_DISPLAY_WIDTH, CAR_ANCHOR_POINT, true);
    img_label_t *car2 = img_label_init(CAR2_FPATH, CAR2_TYPE, CAR2_POS, CAR2_DISPLAY_WIDTH, CAR_ANCHOR_POINT, false);
    list_add(car_list_splayer, car1);
    list_add(car_list_splayer, car2);

    ui_group_add(single_player_car_selection, selection_background);
    ui_group_add(single_player_car_selection, right_arrow);
    ui_group_add(single_player_car_selection, left_arrow);
    ui_group_add(single_player_car_selection, car1);
    ui_group_add(single_player_car_selection, car2);
    ui_group_add(single_player_car_selection, checkmark);

    ui_group_set_callback(single_player_car_selection, right_arrow, CALLBACK_MOUSE_UP, right_arrow_car_splayer, NULL, NULL);
    ui_group_set_callback(single_player_car_selection, left_arrow, CALLBACK_MOUSE_UP, left_arrow_car_splayer, NULL, NULL);
    ui_group_set_callback(single_player_car_selection, checkmark, CALLBACK_MOUSE_UP, splayer_car_selected, NULL, NULL);
}

void make_splayer_map_selection() {
    single_player_map_selection = ui_group_init();
    img_label_t *selection_background = img_label_init(SELECT_MAP_FPATH, SELECT_MAP_TYPE, SELECT_MAP_POS, SELECT_MAP_WIDTH, SELECT_MAP_ANCHOR_POINT, true);
    img_label_t *map1 = img_label_init(MAP1_FPATH, MAP1_TYPE, MAP_POS, MAP_DISPLAY_WIDTH, MAP_ANCHOR_POINT, true);
    img_label_t *map2 = img_label_init(MAP2_FPATH, MAP2_TYPE, MAP_POS, MAP_DISPLAY_WIDTH, MAP_ANCHOR_POINT, false);
    img_label_t *right_arrow = img_label_init(MAP_RIGHT_ARROW_FPATH, MAP_RIGHT_ARROW_TYPE, MAP_RIGHT_ARROW_POS, MAP_RIGHT_ARROW_WIDTH, MAP_RIGHT_ARROW_ANCHOR_POINT, true);
    img_label_t *left_arrow = img_label_init(MAP_LEFT_ARROW_FPATH, MAP_LEFT_ARROW_TYPE, MAP_LEFT_ARROW_POS, MAP_LEFT_ARROW_WIDTH, MAP_LEFT_ARROW_ANCHOR_POINT, true);
    img_label_t *checkmark = img_label_init(MAP_SELECT_CHECKMARK_FPATH, MAP_SELECT_CHECKMARK_TYPE, MAP_SELECT_CHECKMARK_POS, MAP_SELECT_CHECKMARK_WIDTH, MAP_SELECT_CHECKMARK_ANCHOR_POINT, true);

    map_list = list_init(2, (free_func_t)img_label_free);
    list_add(map_list, map1);
    list_add(map_list, map2);

    ui_group_add(single_player_map_selection, selection_background);
    ui_group_add(single_player_map_selection, right_arrow);
    ui_group_add(single_player_map_selection, left_arrow);
    ui_group_add(single_player_map_selection, map1);
    ui_group_add(single_player_map_selection, map2);
    ui_group_add(single_player_map_selection, checkmark);

    ui_group_set_callback(single_player_map_selection, right_arrow, CALLBACK_MOUSE_UP, right_arrow_map, NULL, NULL);
    ui_group_set_callback(single_player_map_selection, left_arrow, CALLBACK_MOUSE_UP, left_arrow_map, NULL, NULL);
    ui_group_set_callback(single_player_map_selection, checkmark, CALLBACK_MOUSE_UP, splayer_map_selected, NULL, NULL);
}

void make_splayer_in_game() {
    single_player_game = ui_group_init();
    img_label_t *timer = img_label_init(SP_TIMER_FPATH, SP_TIMER_TYPE, SP_TIMER_POS, SP_TIMER_WIDTH, SP_TIMER_ANCHOR_POINT, true);
    img_label_t *lap_counter = img_label_init(SP_LAP_COUNTER_FPATH, SP_LAP_COUNTER_TYPE, SP_LAP_COUNTER_POS, SP_LAP_COUNTER_WIDTH, SP_LAP_COUNTER_ANCHOR_POINT, true);
    img_label_t *controls = img_label_init(CONTROLS_FPATH, CONTROLS_TYPE, CONTROLS_POS, CONTROLS_WIDTH, CONTROLS_ANCHOR_POINT, true);

    ui_group_add(single_player_game, timer);
    ui_group_add(single_player_game, lap_counter);
    ui_group_add(single_player_game, controls);
}


// Multiplayer menus
void make_mplayer_car_selection() {
    multi_player_car_selection = ui_group_init();
    img_label_t *selection_background = img_label_init(SELECT_CAR_FPATH, SELECT_CAR_TYPE, SELECT_CAR_POS, SELECT_CAR_WIDTH, SELECT_CAR_ANCHOR_POINT, true);
    img_label_t *right_arrow = img_label_init(RIGHT_ARROW_FPATH, RIGHT_ARROW_TYPE, RIGHT_ARROW_POS, RIGHT_ARROW_WIDTH, RIGHT_ARROW_ANCHOR_POINT, true);
    img_label_t *left_arrow = img_label_init(LEFT_ARROW_FPATH, LEFT_ARROW_TYPE, LEFT_ARROW_POS, LEFT_ARROW_WIDTH, LEFT_ARROW_ANCHOR_POINT, true);
    img_label_t *checkmark = img_label_init(CAR_SELECT_CHECKMARK_FPATH, CAR_SELECT_CHECKMARK_TYPE, CAR_SELECT_CHECKMARK_POS, CAR_SELECT_CHECKMARK_WIDTH, CAR_SELECT_CHECKMARK_ANCHOR_POINT, true);

    car_list_mplayer = list_init(2, (free_func_t)img_label_free);
    img_label_t *car1 = img_label_init(CAR1_FPATH, CAR1_TYPE, CAR1_POS, CAR1_DISPLAY_WIDTH, CAR_ANCHOR_POINT, true);
    img_label_t *car2 = img_label_init(CAR2_FPATH, CAR2_TYPE, CAR2_POS, CAR2_DISPLAY_WIDTH, CAR_ANCHOR_POINT, false);
    list_add(car_list_mplayer, car1);
    list_add(car_list_mplayer, car2);

    ui_group_add(multi_player_car_selection, selection_background);
    ui_group_add(multi_player_car_selection, right_arrow);
    ui_group_add(multi_player_car_selection, left_arrow);
    ui_group_add(multi_player_car_selection, car1);
    ui_group_add(multi_player_car_selection, car2);
    ui_group_add(multi_player_car_selection, checkmark);

    ui_group_set_callback(multi_player_car_selection, right_arrow, CALLBACK_MOUSE_UP, right_arrow_car_mplayer, NULL, NULL);
    ui_group_set_callback(multi_player_car_selection, left_arrow, CALLBACK_MOUSE_UP, left_arrow_car_mplayer, NULL, NULL);
    ui_group_set_callback(multi_player_car_selection, checkmark, CALLBACK_MOUSE_UP, mplayer_car_selected, NULL, NULL);
}

void make_multiplayer_lobby() {
    multi_player_lobby = ui_group_init();
    img_label_t *lobby = img_label_init(MPLAYER_LOBBY_FPATH, MPLAYER_LOBBY_TYPE, MPLAYER_LOBBY_POS, MPLAYER_LOBBY_WIDTH, MPLAYER_ANCHOR_POINT, true);
    ui_group_add(multi_player_lobby, lobby);
}


void make_multiplayer_in_game() {
    multiplayer_game = ui_group_init();
    img_label_t *lap_counter = img_label_init(MP_LAP_COUNTER_FPATH, MP_LAP_COUNTER_TYPE, MP_LAP_COUNTER_POS, MP_LAP_COUNTER_WIDTH, MP_LAP_COUNTER_ANCHOR_POINT, true);
    img_label_t *controls = img_label_init(CONTROLS_FPATH, CONTROLS_TYPE, CONTROLS_POS, CONTROLS_WIDTH, CONTROLS_ANCHOR_POINT, true);

    ui_group_add(multiplayer_game, lap_counter);
    ui_group_add(multiplayer_game, controls);
}

void make_singleplayer_game_over() {
    single_player_game_over = ui_group_init();
    img_label_t *game_over = img_label_init(SPLAYER_GAME_OVER_FPATH, SPLAYER_GAME_OVER_TYPE, SPLAYER_GAME_OVER_POS, SPLAYER_GAME_OVER_WIDTH, SPLAYER_GAME_OVER_ANCHOR_POINT, true);
    ui_group_add(single_player_game_over, game_over);
}

void make_multiplayer_game_over() {
    multi_player_game_over = ui_group_init();
    img_label_t *game_over = img_label_init(MPLAYER_GAME_OVER_FPATH, MPLAYER_GAME_OVER_TYPE, MPLAYER_GAME_OVER_POS, MPLAYER_GAME_OVER_WIDTH, MPLAYER_GAME_OVER_ANCHOR_POINT, true);
    ui_group_add(multi_player_game_over, game_over);
}


// MAIN FUNCTIONS
// list_t players should be passed in as list of ints
list_t *display_lobby_text(list_t *players) {
    list_t *texts = list_init(list_size(players), free);
    rgba_color_t color = (rgba_color_t){ 1.0, 1.0, 1.0, 1.0 };
    //printf("%zu\n", list_size(players));
    for (size_t i = 0; i < list_size(players); i++) {
        int current = *(int *)list_get(players, i);
        printf("id: %i\n", current);
        vector_t pos = vec_add(first_lobby_position, vec_multiply(i, lobby_pos_increment));
        //printf("x: %.9f, y: %.9f\n", pos.x, pos.y);
        char *msg = malloc(sizeof(char) * 20);
        //printf("our id: %i\n", client_get_id(state->client));
        if (current != client_get_id(state->client)) {
            sprintf(msg, "Dominic %d", current);
        }
        else {
            sprintf(msg, "You");
        }
        //printf("we wish to render: %s\n", msg);
        
        list_add(texts, text_label_init(msg, font, pos, lobby_fontsize, true, color));
    }
    return texts;
}

void show_main_menu() {
    ui_group_
    scene_set_ui(state->scene, main_menu);
}

void init_ui_handler(state_t *s) {
    state = s;
    make_main_menu();
    make_splayer_car_selection();
    make_splayer_map_selection();
    make_mplayer_instructions();
    make_splayer_instructions();
    make_splayer_in_game();
    make_multiplayer_in_game();
    make_multiplayer_in_game();
    make_multiplayer_lobby();
    make_mplayer_car_selection();
    make_singleplayer_game_over();
    make_multiplayer_game_over();
}

void ui_update() {
    //printf("current status; %i status_in_lobyy is %i\n", state->status, STATUS_IN_LOBBY);
    if (state->status == STATUS_IN_LOBBY && state->lobby_ids) {
        //printf("status in lobby\n");
        //printf("stuff %p\n", state->lobby_ids);
        // for now disable ui_group_set_texts(multi_player_lobby, display_lobby_text(state->lobby_ids));
        ui_group_set_texts(multi_player_lobby, display_lobby_text(state->lobby_ids));
    }
    else if (state->status == STATUS_IN_LOCAL_GAME) {
        ui_group_clear_texts(single_player_game);
        char *laps_msg = malloc(sizeof(char) * 5);
        char *timer_msg = malloc(sizeof(char) * 10);
        int laps = client_get_laps(state->client)->lap_count;
        double time_remaining = client_get_laps(state->client)->time_remaining;
        int print_laps = laps;
        if (laps < 0) {
            print_laps = 0;
        }
        sprintf(laps_msg, "%d", print_laps);
        sprintf(timer_msg, "%.2f", time_remaining);
        text_label_t *laps_text = text_label_init(laps_msg, font, lap_counter_position, lobby_fontsize, true, (rgba_color_t){ 1.0, 1.0, 1.0, 1.0 });
        text_label_t *timer_text = text_label_init(timer_msg, font, timer_position, lobby_fontsize, true, (rgba_color_t){ 1.0, 1.0, 1.0, 1.0 });
        ui_group_add_text(single_player_game, laps_text);
        ui_group_add_text(single_player_game, timer_text);
    }
    else if (state->status == STATUS_IN_GLOBAL_GAME) {
        if (scene_get_ui(state->scene) != multiplayer_game) {
            scene_set_ui(state->scene, multiplayer_game);
        }
        ui_group_clear_texts(multiplayer_game);
        char *msg = malloc(sizeof(char) * 7);
        int laps = client_get_laps(state->client)->lap_count;
        int print_laps = laps;
        if (laps < 0) {
            print_laps = 0;
        }
        sprintf(msg, "%d/%zu", print_laps, MULTIPLAYER_MAX_LAPS);
        text_label_t *text = text_label_init(msg, font, multiplayer_lap_counter_position, lobby_fontsize, true, (rgba_color_t){ 1.0, 1.0, 1.0, 1.0 });
        ui_group_add_text(multiplayer_game, text);
    }
    else if (state->status == STATUS_SINGLE_PLAYER_GAME_OVER) {
        if (scene_get_ui(state->scene) != single_player_game_over) {
            scene_set_ui(state->scene, single_player_game_over);
        }
        ui_group_clear_texts(single_player_game_over);
        char *laps_msg = malloc(sizeof(char) * 5);
        int laps = client_get_laps(state->client)->lap_count;
        int print_laps = laps;
        if (laps < 0) {
            print_laps = 0;
        }
        sprintf(laps_msg, "%d laps", print_laps);
        text_label_t *laps_text = text_label_init(laps_msg, font, singleplayer_game_over_text_position, game_over_fontsize, true, (rgba_color_t){ 1.0, 1.0, 1.0, 1.0 });
        ui_group_add_text(single_player_game_over, laps_text);
    }
    else if (state->status == STATUS_MULTI_PLAYER_GAME_OVER) {
        if (scene_get_ui(state->scene) != multi_player_game_over) {
            scene_set_ui(state->scene, multi_player_game_over);
        }
        ui_group_clear_texts(multi_player_game_over);
        char *msg = malloc(sizeof(char) * 50);
        int winner_id = state->multi_winner;
        sprintf(msg, "Dominic %i wins!", winner_id);
        text_label_t *text = text_label_init(msg, font, multiplayer_game_over_text_position, multi_player_game_over_fontsize, true, (rgba_color_t){ 1.0, 1.0, 1.0, 1.0 });
        ui_group_add_text(multi_player_game_over, text);
    }
}