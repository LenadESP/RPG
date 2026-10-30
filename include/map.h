#ifndef MAP_H
#define MAP_H

#define TILE_SIZE 16

#include "texture.h"

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
}Map;

void map_load(Map *map, const LayerDef *defs, int count);
void map_unload(Map *map);
void map_draw(const Map *map);
void map_is_solid(void);

#endif