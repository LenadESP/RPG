#ifndef TEXTURE_H
#define TEXTURE_H

#include "raylib.h"

typedef struct TextureAsset {
    Texture2D texture;
    int frame_w;
    int frame_h;
    int off_x;
    int off_y;
    int columns;
    int rows;
} TextureAsset;

typedef enum {
    TEX_NONE,
    TEX_PLAYER,
    TEX_TILESET_FLOOR,
    TEX_TILESET_FLOOR_DETAIL,
    TEX_TILESET_ELEMENT,
    TEX_TILESET_NATURE,
    TEX_TILESET_COLLISIONS,
    TEX_COUNT
} TextureId;

void texture_load_all(void);
void texture_unload_all(void);
TextureAsset texture_get(TextureId id);

#endif