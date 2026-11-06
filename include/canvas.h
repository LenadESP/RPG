#ifndef CANVAS_H
#define CANVAS_H

#include "texture.h"
#include "text.h"
#include "raylib.h"

#include <stdbool.h>

typedef struct CanvasElement {
    TextureId normal_texture;
    Rectangle area;
    bool hover_effect;
    TextureId hover_texture;
} CanvasElement;

typedef struct CanvasElementText{
    Rectangle area;
    bool translated;
    TextureId textures[LANG_COUNT];
} CanvasElementText;

Rectangle canvas_to_screen(Rectangle area, float scale, float offset_x, float offset_y);

#endif