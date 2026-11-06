#include "entity.h"
#include "game.h"
#include "raylib.h"
#include "collision.h"
#include "map.h"
#include "defines.h"
#include "texture.h"
#include "canvas.h"

void game_menu_draw(void (*start_game)(void), void (*close_game)(void)){
    float scale_x = (float)GetScreenWidth()  / CANVAS_WIDTH;
    float scale_y = (float)GetScreenHeight() / CANVAS_HEIGHT;
    float scale   = (scale_x < scale_y) ? scale_x : scale_y; //operador ternario, elige el menor.

    float offset_x = (GetScreenWidth()  - CANVAS_WIDTH * scale) / 2.0f;
    float offset_y = (GetScreenHeight() - CANVAS_HEIGHT * scale) / 2.0f;

    static const CanvasElement UIElements[] = {
        [0] = {TEX_MAINMENU_BG,      {  0.0f,   0.0f, 1672.0f, 941.0f}},
        [1] = {TEX_MAINMENU_LOGO,    {523.5f,  60.0f,  625.0f, 405.0f}},
        [2] = {TEX_MAINMENU_NBUTTON, {576.0f, 520.0f,  520.0f, 100.0f}, true, TEX_MAINMENU_HBUTTON},
        [3] = {TEX_MAINMENU_NBUTTON, {576.0f, 650.0f,  520.0f, 100.0f}, true, TEX_MAINMENU_HBUTTON},
        [4] = {TEX_MAINMENU_NBUTTON, {576.0f, 780.0f,  520.0f, 100.0f}, true, TEX_MAINMENU_HBUTTON},
    };


    /* area is the BOX the label is centered in: same rect as its plaque.
       Actual size is worked out at draw time, because each language's
       word has its own width. */
    static const CanvasElementText UITexts[] = {
        [0] = {{576.0f, 520.0f, 520.0f, 100.0f}, true, {
                   [LANG_ES] = TEX_MAINMENU_TEXT_JUGAR,
                   [LANG_EN] = TEX_MAINMENU_TEXT_PLAY,
                   [LANG_EU] = TEX_MAINMENU_TEXT_JOLASTU}},

        [1] = {{576.0f, 650.0f, 520.0f, 100.0f}, true, {
                   [LANG_ES] = TEX_MAINMENU_TEXT_CONFIGURACION,
                   [LANG_EN] = TEX_MAINMENU_TEXT_CONFIGURATION,
                   [LANG_EU] = TEX_MAINMENU_TEXT_KONFIGURAZIOA}},

        [2] = {{576.0f, 780.0f, 520.0f, 100.0f}, true, {
                   [LANG_ES] = TEX_MAINMENU_TEXT_SALIR,
                   [LANG_EN] = TEX_MAINMENU_TEXT_EXIT,
                   [LANG_EU] = TEX_MAINMENU_TEXT_ITXI}},
    };

        // Mouse comes in screen space. Undo the transform so it speaks canvas and
    // every rect in the tables can be used as written.
    // Subtract the offset FIRST, then divide.
    Vector2 mouse = GetMousePosition();
    Vector2 mouse_canvas = {(mouse.x - offset_x) / scale, (mouse.y - offset_y) / scale};

    for (int i = 0; i < (int)(sizeof(UIElements) / sizeof(UIElements[0])); i++) {
        const CanvasElement *element = &UIElements[i];
        bool hovered = element->hover_effect && CheckCollisionPointRec(mouse_canvas, element->area);
        TextureAsset asset = texture_get(hovered ? element->hover_texture : element->normal_texture);

        DrawTexturePro(asset.texture,
                       (Rectangle){0, 0, asset.frame_w, asset.frame_h},
                       canvas_to_screen(element->area, scale, offset_x, offset_y),
                       (Vector2){0, 0}, 0, WHITE);
    }

    // TODO: this comes from the translation system (card 32) once it exists.
    Language current_language = LANG_ES;

    for (int i = 0; i < (int)(sizeof(UITexts) / sizeof(UITexts[0])); i++) {
        const CanvasElementText *label = &UITexts[i];

        // Untranslated labels (proper nouns) always live at index 0.
        TextureId label_id = label->translated ? label->textures[current_language]
                                               : label->textures[LANG_ES];
        TextureAsset asset = texture_get(label_id);

        // Fixed height, width from the texture's own ratio, so every language
        // reads at the same size and none of them get squashed.
        float label_h = MENU_TEXT_HEIGHT;
        float label_w = label_h * asset.frame_w / asset.frame_h;

        Rectangle label_area = {label->area.x + (label->area.width  - label_w) / 2.0f,
                                label->area.y + (label->area.height - label_h) / 2.0f,
                                label_w, label_h};

        DrawTexturePro(asset.texture,
                       (Rectangle){0, 0, asset.frame_w, asset.frame_h},
                       canvas_to_screen(label_area, scale, offset_x, offset_y),
                       (Vector2){0, 0}, 0, WHITE);
    }

    // Plaques are UIElements 2, 3 and 4.
    // If you reorder that table, these indices lie and nothing warns you.
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse_canvas, UIElements[2].area)) {
            start_game();
        } else if (CheckCollisionPointRec(mouse_canvas, UIElements[3].area)) {
            // CONFIG: nothing yet
        } else if (CheckCollisionPointRec(mouse_canvas, UIElements[4].area)) {
            close_game();
        }
    }

}

void game_pause_draw(const Entity *entities, int count, bool debug, Camera2D *camera, const Map *map){
    game_draw(entities, count, debug, camera, map);

    int size = 40;
    const char *txt = "Paused! Press ESC to resume";
    DrawText(txt, GetScreenWidth()/2 - MeasureText(txt, size)/2, 500, size, BLACK);    
}

void game_draw(const Entity *entities, int count, bool debug, Camera2D *camera, const Map *map){
    BeginMode2D(*camera);

    map_draw(map, 0, map->layer_cut);
    
    for (int i = 0; i < count; i++) {
        if (!entities[i].active) continue;

        entity_draw(&entities[i]);

        //Drawing the hitbox
        if(debug){
            DrawRectangleLinesEx(collision_rect(&entities[i]), 1, YELLOW);
        }
    }

    map_draw(map, map->layer_cut, map->layer_amount);

    EndMode2D();
}