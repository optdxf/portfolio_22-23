#ifndef __MAP_H__
#define __MAP_H__

#include "list.h"
#include "vector.h"

typedef struct map map_t;
typedef enum map_type { MAP_ONE, MAP_TWO } map_type_t;

/**
Initializes map based on type
* @return the map_t object
**/

map_t *map_init(map_type_t type);

/**
* @param idx the index of the player
* @return the map_spawn location (dependent on multiplayer)
**/
vector_t map_get_spawn(map_t *map, size_t idx);

/**
* @return list of objects in the map
**/
list_t *map_get_objects(map_t *map);

/**
* @returns map image
**/
const char *map_get_image(map_t *map);

/**
* @return the map type
**/
map_type_t map_get_type(map_t *map);

/**
* @return the scaling constant for a map
**/
double map_get_scaling(map_t *map);
#endif