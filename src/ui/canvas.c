#include "canvas.h"
#include "raylib.h"
#include "texture.h"
#include "defines.h"
#include <stdlib.h>
#include <stdbool.h>

//The biggest scale where the whole canvas fits, centered. The rest is black bars
CanvasTransform canvas_get_transform(void){
    float scale_x = (float)GetScreenWidth()  / CANVAS_WIDTH;
    float scale_y = (float)GetScreenHeight() / CANVAS_HEIGHT;
    float scale   = (scale_x < scale_y) ? scale_x : scale_y; //operador ternario, elige el menor.

    CanvasTransform transform = {
        .scale    = scale,
        .offset_x = (GetScreenWidth()  - CANVAS_WIDTH * scale) / 2.0f,
        .offset_y = (GetScreenHeight() - CANVAS_HEIGHT * scale) / 2.0f,
    };

    return transform;
}

// Mouse comes in screen space. Undo the transform so it speaks canvas and
// every rect in the tables can be used as written.
// Subtract the offset FIRST, then divide.
Vector2 canvas_mouse(CanvasTransform transform){
    Vector2 mouse = GetMousePosition();
    Vector2 mouse_canvas = {(mouse.x - transform.offset_x) / transform.scale, (mouse.y - transform.offset_y) / transform.scale};

    return mouse_canvas;
}

Rectangle canvas_to_screen(Rectangle area, CanvasTransform transform)
{
    return (Rectangle){
        transform.offset_x + area.x * transform.scale,
        transform.offset_y + area.y * transform.scale,
        area.width  * transform.scale,
        area.height * transform.scale
    };
}


void draw_glitched(TextureId id, Rectangle area, float alpha, float amount,
                   CanvasTransform transform){
    TextureAsset asset = texture_get(id);

    //Each boundary comes from its own division so the strips tile exactly.
    //A gap here would still show at the end, when it must look like one draw
    for (int i = 0; i < LOADING_STRIPS; i++) {
        float sy0 = asset.frame_h * (float)i       / LOADING_STRIPS;
        float sy1 = asset.frame_h * (float)(i + 1) / LOADING_STRIPS;
        float dy0 = area.y + area.height * (float)i       / LOADING_STRIPS;
        float dy1 = area.y + area.height * (float)(i + 1) / LOADING_STRIPS;

        //Cubed so most strips barely move and the odd one flies off
        float u  = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        float dx = (((float)rand() / RAND_MAX) < 0.55f) ? u * u * u * amount : 0.0f;

        Rectangle src = {0, sy0, asset.frame_w, sy1 - sy0};
        Rectangle dst = {area.x + dx, dy0, area.width, dy1 - dy0};

        DrawTexturePro(asset.texture, src,
                       canvas_to_screen(dst, transform),
                       (Vector2){0, 0}, 0, Fade(WHITE, alpha));
    }
}

void canvas_draw_texture(TextureId id, Rectangle area, CanvasTransform transform){
    TextureAsset asset = texture_get(id);

    DrawTexturePro(asset.texture,
                   (Rectangle){0, 0, asset.frame_w, asset.frame_h},
                   canvas_to_screen(area, transform),
                   (Vector2){0, 0}, 0, WHITE);
}

//Returns the index of the hovered element, or -1 if there's none
int canvas_draw_elements(const CanvasElement *elements, int count, Vector2 mouse_canvas, CanvasTransform transform){
    int hovered_index = -1;

    for (int i = 0; i < count; i++) {
        bool hovered = elements[i].hover_effect && CheckCollisionPointRec(mouse_canvas, elements[i].area);

        if (hovered) {
            hovered_index = i;
            canvas_draw_texture(elements[i].hover_texture, elements[i].area, transform);
        } else {
            canvas_draw_texture(elements[i].normal_texture, elements[i].area, transform);
        }
    }

    return hovered_index;
}

void canvas_draw_texts(const CanvasElementText *texts, int count, CanvasTransform transform){
    for (int i = 0; i < count; i++) {
        text_draw(text_get(texts[i].text), texts[i].style, texts[i].area, transform);
    }
}
