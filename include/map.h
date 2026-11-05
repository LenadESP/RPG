#ifndef MAP_H
#define MAP_H

#include "texture.h"
#include "defines.h"
#include <stdbool.h>

typedef struct LayerDef {
    const char *path;
    TextureId tex;
} LayerDef;

typedef struct Layer{
    int *tiles;
    TextureId tex;
} Layer;

typedef struct Map{
    int w;
    int h;
    Layer *layers;
    int layer_amount;
    int layer_cut;
    int *collision; // one tile per cell, -1 = walkable
}Map;

void map_load(Map *map, const LayerDef *defs, int count, int layer_cut, const char *collision_path);
void map_unload(Map *map);
void map_draw(const Map *map, int first, int last);
bool map_is_solid(const Map *map, int tile_x, int tile_y);

#endif