#ifndef __SDL_WRAPPER2_H__
#define __SDL_WRAPPER2_H__

#include "img_label.h"
#include "SDL2/SDL.h"
#include <SDL2/SDL_ttf.h>
#include "sdl_constants.h"
#include <stdbool.h>
#include "body.h"
#include "scene.h"
#include "color.h"

//---------------

/**
 * Returns the ASCII value corresponding to a specific key
**/
char get_keycode(SDL_Keycode key);

/**
 * Translating from scene to window coordinates
**/
vector_t scene_to_window_pos(vector_t pos);

/**
 * Translating from window to scene coordinates
**/
vector_t window_to_scene_pos(vector_t pos);

/**
 * Translating sizes from scene to window change-of-coordinates
**/
vector_t scene_to_window_size(vector_t size);

/**
 * Translating sizes from window to scene change-of-coordinates
**/
vector_t window_to_scene_size(vector_t size);

/**
 * Draws a polygon from the given list of vertices and a color.
 *
 * @param points the list of vertices of the polygon
 * @param color the color used to fill in the polygon
 */
void sdl_draw_polygon(list_t *points, rgba_color_t color);

/**
 * Sets key handler for SDL_wrapper
**/
void sdl_set_key_handler(key_handler_t handler);

/**
 * Sets mouse handler for SDL_wrapper
**/
void sdl_set_mouse_handler(mouse_handler_t handler);

/**
 * Caches the given image in the filepath as a texture. If this is the first time the texture
 * has been accessed, the image is loaded from disk and cached. If this is not the first
 * time, the associated texture will be accessed from a hash table.
**/
SDL_Texture *sdl_get_texture(const char *fpath, image_type_t type);

/**
 * Initializes the SDL window and renderer.
 * Must be called once before any of the other SDL functions.
 */
void sdl_init();

/**
 * Clears the screen. Should be called before drawing polygons in each frame.
 */
void sdl_clear();

/**
 * Displays the rendered frame on the SDL window.
 * Must be called after drawing the polygons in order to show them.
 */
void sdl_show();

/**
 * Given the position, the TTF_Font, and the associated properties, generates the texture
 * of the text.
 * @param rect pointer to SDL_Rect passed in, and the rect is edited
**/
SDL_Texture *get_texture_and_rectangle(vector_t pos, char *str, TTF_Font *font, rgba_color_t color, SDL_Rect *rect);

/**
 * Renders text of a given font at a given position
 * @param str the string to render
 * @param pos the upper left corner of the message to display
 * @param char* filepath of the font to use
 * @param int size of the font
 * @param rgba_color_t color of the font to display
**/
void sdl_render_text(char *str, vector_t pos, char *font_fpath, int font_size, rgba_color_t color);

/**
 * Renders the map on the scene
**/
void sdl_render_scene_map(scene_t *scene);

/**
 * Renders the bodies on the scene
**/
void sdl_render_scene_bodies(scene_t *scene, body_t *last);

/**
 * Renders the UI on the scene
**/
void sdl_render_scene_ui(scene_t *scene);

/**
 * Renders the scene's map first, then the bodies, then the UI, with the body_t last body
 * being rendered last
**/
void sdl_render_scene_with_last(scene_t *scene, body_t *last);

/**
 * Renders the scene's map first, then the bodies, then the UI, with the body_t last body
 * being rendered last, with the only exception that the camera width and camera height
 * are given to display only a subset of the entire scene
 * @param scene* scene in question
 * @param body_t* last to render
 * @param camera_width in window coordinates
 * @param camera_height in window coordinates
**/
void sdl_render_partial_scene_with_last(scene_t *scene, body_t *last, double camera_width, double camera_height);

/**
 * Processes all SDL events and returns whether the window has been closed.
 * This function must be called in order to handle keypresses.
 *
 * @param void* optional data to pass to key handler
 * @return true if the window was closed, false otherwise
 */
bool sdl_is_done(void *data);

#endif