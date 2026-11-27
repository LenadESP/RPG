#include "canvas.h"
#include "raylib.h"
#include "texture.h"
#include "defines.h"
#include <stdlib.h>
#include <stdbool.h>

Rectangle canvas_to_screen(Rectangle area, float scale, float offset_x, float offset_y)
{
    return (Rectangle){
        offset_x + area.x * scale,
        offset_y + area.y * scale,
        area.width  * scale,
        area.height * scale
    };
}


void draw_glitched(TextureId id, Rectangle area, float alpha, float amount,
                          float scale, float offset_x, float offset_y){
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
                       canvas_to_screen(dst, scale, offset_x, offset_y),
                       (Vector2){0, 0}, 0, Fade(WHITE, alpha));
    }
}

void canvas_draw_texture(TextureId id, Rectangle area, float scale, float offset_x, float offset_y){
    TextureAsset asset = texture_get(id);

    DrawTexturePro(asset.texture,
                   (Rectangle){0, 0, asset.frame_w, asset.frame_h},
                   canvas_to_screen(area, scale, offset_x, offset_y),
                   (Vector2){0, 0}, 0, WHITE);
}

//Returns the index of the hovered element, or -1 if there's none
int canvas_draw_elements(const CanvasElement *elements, int count, Vector2 mouse_canvas, float scale, float offset_x, float offset_y){
    int hovered_index = -1;

    for (int i = 0; i < count; i++) {
        bool hovered = elements[i].hover_effect && CheckCollisionPointRec(mouse_canvas, elements[i].area);

        if (hovered) {
            hovered_index = i;
            canvas_draw_texture(elements[i].hover_texture, elements[i].area, scale, offset_x, offset_y);
        } else {
            canvas_draw_texture(elements[i].normal_texture, elements[i].area, scale, offset_x, offset_y);
        }
    }

    return hovered_index;
}

void canvas_draw_texts(const CanvasElementText *texts, int count, float scale, float offset_x, float offset_y){
    for (int i = 0; i < count; i++) {
        text_draw(text_get(texts[i].text), texts[i].style, texts[i].area, scale, offset_x, offset_y);
    }
}
