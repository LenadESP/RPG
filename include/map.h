#ifndef MAP_H
#define MAP_H

#include "texture.h"

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

void map_load(Map *map);
void map_unload(Map *map);
void map_draw(Map *map);
void map_is_solid(void);

#endif