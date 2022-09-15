#ifndef __UI_HANDLER_H__
#define __UI_HANDLER_H__

#include "ui_group.h"
#include "client_launcher.h"

/**
 * Callback for selecting single player mode
 * Displays single player instructions
**/
void splayer_selected(img_label_t *button, void *data);

/**
 * Callback for confirming single player mode
 * Displays single player car selection
**/
void splayer_instructions_next(img_label_t *button, void *data);

/**
 * Callback for returning to main menu
 * Displays main menu
**/
void splayer_instructions_back(img_label_t *button, void *data);

/**
 * Callback for confirming multiplayer mode
 * Displays multiplayer car selection
**/
void mplayer_instructions_next(img_label_t *button, void *data);

/**
 * Callback for returning to main menu
 * Displays main menu
**/
void mplayer_instructions_back(img_label_t *button, void *data);

/**
 * Callback for selecting multiplayer mode
 * Displays multiplayer instructions
**/
void mplayer_selected(img_label_t *button, void *data);

/**
 * Callback for selecting map1 in single player mode
 * Begins game
**/
void splayer_map1_selected(img_label_t *map1, void *data);

/**
 * Callback for selecting map2 in single player mode
 * Begins game
**/
void splayer_map2_selected(img_label_t *map2, void *data);

/**
 * Callback for toggling map to next map
 * Displays next map in selection menu
**/
void right_arrow_car_splayer(img_label_t *button, void *data);

/**
 * Callback for toggling map to previous map
 * Displays previous map in selection menu
**/
void left_arrow_car_splayer(img_label_t *button, void *data);

/**
 * Callback for toggling car to next car
 * Displays next car in selection menu
**/
void right_arrow_car_mplayer(img_label_t *button, void *data);

/**
 * Callback for toggling car to previous car
 * Displays previous car in selection menu
**/
void left_arrow_car_mplayer(img_label_t *button, void *data);

/**
 * Callback for selecting car; passes information to state
**/
void splayer_car_selected(img_label_t *button, void *data);

/**
 * Callback for toggling map to next map
 * Displays next map in selection menu
**/
void right_arrow_map(img_label_t *button, void *data);

/**
 * Callback for toggling map to previous map
 * Displays previous map in selection menu
**/
void left_arrow_map(img_label_t *button, void *data);

/**
 * Callback for selecting map; passes information to state
**/
void splayer_map_selected(img_label_t *button, void *data);

/**
 * Callback for selecting multiplayer car; passes information to state
**/
void mplayer_car_selected(img_label_t *button, void *data);

/**
 * Initializes UI handler and associates it with state; makes all the menus
**/
void init_ui_handler(state_t *state);

/**
 * Makes main menu and instantiates the associated objects: img_label_ts, etc.
 * NOTE: All "make___()" funtions below are analogous; comments excluded for brevity
**/
void make_main_menu();

void make_splayer_instructions();

void make_mplayer_instructions();

void make_splayer_car_selection();

void make_splayer_map_selection();

void make_splayer_in_game();

void make_mplayer_car_selection();

void make_multiplayer_lobby();

void make_multiplayer_in_game();

void make_singleplayer_game_over();

void make_multiplayer_game_over();

/**
 * Displays main menu
**/
void show_main_menu();

/**
 * Dynamically updates the UI state based on the game settings
**/
void ui_update();

/**
 * Helper function: given a list of player ids, outputs a list of text_label_t objects
 * to render the associated text for the player lobby
**/
list_t *display_lobby_text(list_t *players);

#endif