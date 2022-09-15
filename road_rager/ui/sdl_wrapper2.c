#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#define NANOSVG_ALL_COLOR_KEYWORDS

#include "sdl_wrapper2.h"
#include "global_constants.h"
#include "uthash.h"
#include "math_util.h"
#include <string.h>
#include <stdio.h>
#include "nanosvg.h"
#include "nanosvgrast.h"
#include "scene.h"
#include "forces.h"
#include "physics_constants.h"
#include "text_label.h"
#include <math.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_image.h>
#include <assert.h>
#include <stdbool.h>
#include <float.h>

#define CHECK_SDL_VALID_STRICT(x) if (!(x)) { \
    printf("[sdl_wrapper2.c] " #x " is null. SDL Error: %s\n", SDL_GetError()); \
    exit(1); \
}

#define CHECK_VALID_STRICT(x) if (!(x)) { \
    printf("[sdl_wrapper2.c] " #x " is null.\n"); \
    exit(1); \
}

//----------------

typedef struct {
	const char *filepath;	// key
	SDL_Texture *texture; // value
	UT_hash_handle hh; // makes structure hashable
} texture_cache_obj_t;

const size_t RAST_BYTES_PER_PIXEL = 4;
const size_t RAST_DOTS_PER_INCH = 96;
const size_t BITS_PER_BYTE = 8;
const uint32_t RAST_RED_MASK = 0x000000FF;
const uint32_t RAST_GREEN_MASK = 0x0000FF00;
const uint32_t RAST_BLUE_MASK = 0x00FF0000;
const uint32_t RAST_ALPHA_MASK = 0xFF000000;

const bool DRAW_WALL = true;
const bool DRAW_GRASS = false;
const bool TEXTURE_DEBUG = false;

//-----------------

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
NSVGrasterizer *rasterizer = NULL;
texture_cache_obj_t *texture_cache = NULL;

key_handler_t key_handler = NULL;
mouse_handler_t mouse_handler = NULL;

double scene_to_window_scale = 0;

//------------------
// UTILITY FUNCTIONS

// allows easy support for left/right/up/down
char get_keycode(SDL_Keycode key) {
	switch (key) {
		case SDLK_LEFT: return LEFT_ARROW;
		case SDLK_UP: return UP_ARROW;
		case SDLK_RIGHT: return RIGHT_ARROW;
		case SDLK_DOWN: return DOWN_ARROW;
		default:
			// Only process 7-bit ASCII characters
			return key == (SDL_Keycode)(char)key ? key : '\0';
	}
}

vector_t scene_to_window_pos(vector_t pos) {
    // note: in the window, y starts at top instead of bottom
    return (vector_t) { pos.x / scene_to_window_scale, WINDOW_HEIGHT - pos.y / scene_to_window_scale };
}

vector_t window_to_scene_pos(vector_t pos) {
    return (vector_t) { pos.x * scene_to_window_scale, (WINDOW_HEIGHT - pos.y) * scene_to_window_scale };
}

vector_t scene_to_window_size(vector_t size) {
    // note: in the window, y starts at top instead of bottom
    return (vector_t) { size.x / scene_to_window_scale, size.y / scene_to_window_scale };
}

vector_t window_to_scene_size(vector_t size) {
    return (vector_t) { size.x * scene_to_window_scale, size.y * scene_to_window_scale };
}

//------------------
// SDL FUNCTIONS

void sdl_draw_polygon(list_t *points, rgba_color_t color) {
	// Check parameters
	size_t n = list_size(points);
	assert(n >= 3);
	assert(0 <= color.r && color.r <= 1);
	assert(0 <= color.g && color.g <= 1);
	assert(0 <= color.b && color.b <= 1);

	// Convert each vertex to a point on screen
	int16_t *x_points = malloc(sizeof(*x_points) * n),
			*y_points = malloc(sizeof(*y_points) * n);
	assert(x_points != NULL);
	assert(y_points != NULL);
	for (size_t i = 0; i < n; i++) {
		vector_t *vertex = list_get(points, i);
        vector_t pixel = scene_to_window_pos(*vertex);
		x_points[i] = pixel.x;
		y_points[i] = pixel.y;
	}

	// Draw polygon with the given color
	filledPolygonRGBA(renderer, x_points, y_points, n, color.r * 255, color.g * 255,
		color.b * 255, color.a * 255);
	free(x_points);
	free(y_points);
}

void sdl_set_key_handler(key_handler_t handler) { key_handler = handler; }
void sdl_set_mouse_handler(mouse_handler_t handler) { mouse_handler = handler; }

SDL_Texture *sdl_get_texture(const char *fpath, image_type_t type) {
    // check cache
    if (TEXTURE_DEBUG) {
        printf("getting texture of: %s\n", fpath);
    }
    texture_cache_obj_t *cached;
    HASH_FIND_STR(texture_cache, fpath, cached);
    if (cached) {
        //printf("found cached texture for: %s\n", fpath);
        return cached->texture;
    } else {
        SDL_Texture *texture = NULL;

        switch (type) {
            case IMG_TYPE_PNG: {
                SDL_Surface *surface = IMG_Load(fpath);
                CHECK_SDL_VALID_STRICT(surface);

                texture = SDL_CreateTextureFromSurface(renderer, surface);
                CHECK_SDL_VALID_STRICT(texture);
                SDL_FreeSurface(surface);
                break;
            }
            case IMG_TYPE_SVG: {
                NSVGimage *image = nsvgParseFromFile(fpath, "px", RAST_DOTS_PER_INCH);
                CHECK_VALID_STRICT(image);

                int width = (int)image->width;
                int height = (int)image->height;

                unsigned char *image_data = malloc(width * height * RAST_BYTES_PER_PIXEL);
                CHECK_VALID_STRICT(image_data);
                nsvgRasterize(rasterizer, image, 0, 0, 1, image_data, width, height, width * RAST_BYTES_PER_PIXEL);

                SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(image_data, width, height,
                    BITS_PER_BYTE * RAST_BYTES_PER_PIXEL,
                    width * RAST_BYTES_PER_PIXEL,
                    RAST_RED_MASK, RAST_GREEN_MASK, RAST_BLUE_MASK, RAST_ALPHA_MASK);
                CHECK_SDL_VALID_STRICT(surface);

                texture = SDL_CreateTextureFromSurface(renderer, surface);
                CHECK_SDL_VALID_STRICT(texture);

                SDL_FreeSurface(surface);
                nsvgDelete(image);
                free(image_data);

                break;
            }
        }
        
        // cache texture
        cached = malloc(sizeof(*cached));
        cached->filepath = fpath;
        cached->texture = texture;
        HASH_ADD_KEYPTR(hh, texture_cache, fpath, strlen(fpath), cached);

        return texture;
    }
}

bool sdl_is_done(void *data) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: return true;
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                if (!key_handler) break;
                SDL_KeyboardEvent kevent = event.key;
                char key = get_keycode(kevent.keysym.sym);
                if (!key) break;

                key_handler(key, event.type == SDL_KEYDOWN ? EVENT_KEY_PRESSED : EVENT_KEY_RELEASED, data);
                break;
            }
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEBUTTONDOWN: {
                if (!mouse_handler) break;
                SDL_MouseButtonEvent mbevent = event.button;
                if (mbevent.button != SDL_BUTTON_LEFT) break;   // only support left right now
                vector_t scene_pos = window_to_scene_pos((vector_t){ mbevent.x, mbevent.y });
                mouse_handler(scene_pos.x, scene_pos.y, event.type == SDL_MOUSEBUTTONDOWN ? EVENT_MOUSE_PRESSED : EVENT_MOUSE_RELEASED, data);
                break;
            }
            case SDL_MOUSEMOTION: {
                if (!mouse_handler) break;
                SDL_MouseMotionEvent mmevent = event.motion;
                vector_t scene_pos = window_to_scene_pos((vector_t){ mmevent.x, mmevent.y });
                mouse_handler(scene_pos.x, scene_pos.y, EVENT_MOUSE_MOVE, data);
                break;
            }
        }
    }
    return false;
}

void sdl_init(vector_t scene_size) {
    // ensure aspect of ratio of scene matches w/ window
    double win_aspect_ratio = WINDOW_WIDTH / WINDOW_HEIGHT;
    double scene_aspect_ratio = scene_size.x / scene_size.y;
    assert("aspect ratios of window and scene do not match" && isclose(win_aspect_ratio, scene_aspect_ratio));
    scene_to_window_scale = scene_size.x / WINDOW_WIDTH;

	SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();
	window = SDL_CreateWindow(GAME_NAME, SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_BORDERLESS);
    CHECK_SDL_VALID_STRICT(window);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    CHECK_SDL_VALID_STRICT(renderer);
    rasterizer = nsvgCreateRasterizer();
}

void sdl_clear() {
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer);
}

void sdl_show() {
    SDL_RenderPresent(renderer);
}

SDL_Texture *get_texture_and_rectangle(
	vector_t pos, char *str, TTF_Font *font, rgba_color_t color, SDL_Rect *rect) {
	SDL_Color textColor = { color.r * 255, color.g * 255, color.b * 255, 255 };

	SDL_Surface *surface = TTF_RenderText_Solid(font, str, textColor);
	SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
	size_t text_width = surface->w;
	size_t text_height = surface->h;
	SDL_FreeSurface(surface);
	rect->x = pos.x;
	rect->y = pos.y;
	rect->w = text_width;
	rect->h = text_height;

	return texture;
}

void sdl_render_text(
	char *str, vector_t pos, char *font_fpath, int font_size, rgba_color_t color) {
	TTF_Font *font = TTF_OpenFont(font_fpath, font_size);
	assert(font);

	SDL_Rect *rect = malloc(sizeof(SDL_Rect));
	SDL_Texture *texture = get_texture_and_rectangle(pos, str, font, color, rect);

	int res1 = SDL_SetRenderDrawColor(renderer, color.r * 255, color.g * 255, color.b * 255, 255);
	SDL_RenderCopy(renderer, texture, NULL, rect) != 0;
	// sdl_show();

	SDL_DestroyTexture(texture);
	free(rect);
	TTF_CloseFont(font);
}

void sdl_render_scene_map(scene_t *scene) {
    img_label_t *map = scene_get_map(scene);
    SDL_Texture *texture = img_label_get_texture(map);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
}

void sdl_render_body(body_t *body) {
    list_t *shape = body_get_shape(body);
	if (body_get_img(body) && body_has_bounding_rect(body)) {
		list_t *box = body_get_bounding_rect(body);
		SDL_Texture *texture = sdl_get_texture(body_get_img(body), IMG_TYPE_PNG);

		// Find corners to show
		double x_max = -DBL_MAX;
		double y_max = -DBL_MAX;
		double x_min = DBL_MAX;
		double y_min = DBL_MAX;
		for (size_t j = 0; j < list_size(box); j++) {
			vector_t *vertex = (vector_t *)list_get(box, j);
			if (vertex->x > x_max) {
				x_max = vertex->x;
			}
			if (vertex->x < x_min) {
				x_min = vertex->x;
			}
			if (vertex->y > y_max) {
				y_max = vertex->y;
			}
			if (vertex->y < y_min) {
				y_min = vertex->y;
			}
		}
		vector_t upper_left = { x_min, y_max };
		vector_t lower_right = { x_max, y_min };

        vector_t window_upper_left = scene_to_window_pos(upper_left);
        vector_t window_lower_right = scene_to_window_pos(lower_right);

		int x = (int)(window_upper_left.x);
		int y = (int)(window_upper_left.y);
		int width = (int)fabs(x - window_lower_right.x);
	    int height = (int)fabs(y - window_lower_right.y);

		SDL_Rect image_rect = { x, y, width, height };
        //printf("x: %i | y: %i | w: %i | h: %i\n", x, y, width, height);
		SDL_RenderCopyEx(renderer, texture, NULL, &image_rect,
			-body_get_rotation(body) * 180 / M_PI, NULL, SDL_FLIP_NONE);
		list_free(box);
	} else {
        if (body_get_info(body)) {
            if (((collision_ref_t *)body_get_info(body))->body_type == WALL) {
                if (DRAW_WALL) {
                    sdl_draw_polygon(shape, body_get_color(body));
                }
            } else if (((collision_ref_t *)body_get_info(body))->body_type == GRASS) {
                if (DRAW_GRASS) {
                    sdl_draw_polygon(shape, body_get_color(body));
                }
            } else {
                sdl_draw_polygon(shape, body_get_color(body));
            }
        } else {
            sdl_draw_polygon(shape, body_get_color(body));
        }
	}
}

void sdl_render_scene_bodies(scene_t *scene, body_t *last) {
    size_t body_count = scene_bodies(scene);
	bool do_render_last = false;
	for (size_t i = 0; i < body_count; i++) {
		body_t *body = scene_get_body(scene, i);
		if (body == last) {
			do_render_last = true;
			continue;
		}
		sdl_render_body(body);
	}
	if (do_render_last) {
        sdl_render_body(last);
	}
}

void sdl_render_scene_ui(scene_t *scene) {
    ui_group_t *group = scene_get_ui(scene);
    list_t *guis = ui_group_get_guis(group);
    list_t *texts = ui_group_get_texts(group);
    // draw images
    for (size_t i = 0; i < list_size(guis); ++i) {
        img_label_t *img = (img_label_t*)list_get(guis, i);
        if (img_label_get_visible(img)) {
            // draw it
            // no rotation support needed atm
            vector_t absolute_pos = scene_to_window_pos(img_label_get_absolute_pos(img));
            vector_t size = scene_to_window_size(img_label_get_size(img));
            SDL_Rect dest = (SDL_Rect) { (int)absolute_pos.x, (int)absolute_pos.y, (int)size.x, (int)size.y };
            SDL_RenderCopy(renderer, img_label_get_texture(img), NULL, &dest);
        }
    }

    // render text
    for (size_t i = 0; i < list_size(texts); ++i) {
        text_label_t *text = (text_label_t*)list_get(texts, i);
        if (text_label_get_visible(text)) {
            vector_t absolute_pos = scene_to_window_pos(text_label_get_position(text));
            //printf("absolutepos-x: %.9f y: %.9f\n", absolute_pos.x, absolute_pos.y);
            //printf("msg: %s\n", text_label_get_message(text));
            //printf("size: %i\n", text_label_get_fontsize(text));
            sdl_render_text(text_label_get_message(text), absolute_pos, text_label_get_font(text), text_label_get_fontsize(text), text_label_get_color(text));
        }
    }

}

void sdl_render_scene_with_last(scene_t *scene, body_t *last) {
	sdl_clear();
	
	if (scene_get_map(scene)) sdl_render_scene_map(scene);
    sdl_render_scene_bodies(scene, last);
    if (scene_get_ui(scene)) sdl_render_scene_ui(scene);

    sdl_show();
}

void sdl_render_partial_scene_with_last(scene_t *scene, body_t *last, double camera_width, double camera_height) {
    // Add everything to texture and then render subset of the texture
	SDL_Texture *universe = SDL_CreateTexture(renderer, SDL_GetWindowPixelFormat(window),
		SDL_TEXTUREACCESS_TARGET, WINDOW_WIDTH, WINDOW_HEIGHT);
	CHECK_SDL_VALID_STRICT(universe);
	SDL_SetRenderTarget(renderer, universe);

    // Clear background
	sdl_clear();

    // Set up camera
	vector_t camera_center = scene_to_window_pos(body_get_centroid(last));    
	SDL_Rect camera = { camera_center.x - camera_width / 2,
		camera_center.y - camera_height / 2, 
        camera_width,
		camera_height
    };

	// Adjust if camera out-of-bounds
	if (camera.x < 0) {
		camera.x = 0;
	} else if (camera.x > WINDOW_WIDTH - camera.w) {
		camera.x = WINDOW_WIDTH - camera.w;
	}
	if (camera.y < 0) {
		camera.y = 0;
	} else if (camera.y > WINDOW_HEIGHT - camera.h) {
		camera.y = WINDOW_HEIGHT - camera.h;
	}

    // Rendered with camera
    if (scene_get_map(scene)) sdl_render_scene_map(scene);
    sdl_render_scene_bodies(scene, last);

    SDL_SetRenderTarget(renderer, NULL);
    
    // Display
    SDL_RenderCopyEx(renderer, universe, &camera, NULL, 0, NULL, SDL_FLIP_NONE);
    SDL_DestroyTexture(universe);

    // UI rendered without camera
    if (scene_get_ui(scene)) sdl_render_scene_ui(scene);
 
    sdl_show();
}