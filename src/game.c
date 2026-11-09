#include "entity.h"
#include "game.h"
#include "raylib.h"
#include "collision.h"
#include "map.h"
#include "texture.h"
#include "canvas.h"
#include "defines.h"

#include <stdlib.h>
#include <stdbool.h>

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
        TextureAsset asset = texture_get(hovered ? element->hover_texture : element->normal_texture); //get either the normal or hovered texture

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

void game_pause_draw(const Entity *entities, int count, bool debug, Camera2D *camera, const Map *map,
                     void (*resume_game)(void), void (*exit_to_menu)(void)){
    game_draw(entities, count, debug, camera, map);

    //Tint over the whole window, outside BeginMode2D
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.6f));

    //Canvas scaling
    float scale_x = (float)GetScreenWidth()  / CANVAS_WIDTH;
    float scale_y = (float)GetScreenHeight() / CANVAS_HEIGHT;
    float scale   = (scale_x < scale_y) ? scale_x : scale_y;

    float offset_x = (GetScreenWidth()  - CANVAS_WIDTH * scale) / 2.0f;
    float offset_y = (GetScreenHeight() - CANVAS_HEIGHT * scale) / 2.0f;

    //No logo here, so the plaques sit centered
    static const CanvasElement UIElements[] = {
        [0] = {TEX_MAINMENU_NBUTTON, {576.0f, 290.0f, 520.0f, 100.0f}, true, TEX_MAINMENU_HBUTTON},
        [1] = {TEX_MAINMENU_NBUTTON, {576.0f, 420.0f, 520.0f, 100.0f}, true, TEX_MAINMENU_HBUTTON},
        [2] = {TEX_MAINMENU_NBUTTON, {576.0f, 550.0f, 520.0f, 100.0f}, true, TEX_MAINMENU_HBUTTON},
    };

    //TEX_NONE: resume and exit-to-menu art doesn't exist yet
    static const CanvasElementText UITexts[] = {
        [0] = {{576.0f, 290.0f, 520.0f, 100.0f}, true, {
                   [LANG_ES] = TEX_MAINMENU_TEXT_VOLVER_AL_JUEGO,
                   [LANG_EN] = TEX_MAINMENU_TEXT_RESUME,
                   [LANG_EU] = TEX_MAINMENU_TEXT_ITZULI_JOKURA}},

        [1] = {{576.0f, 420.0f, 520.0f, 100.0f}, true, {
                   [LANG_ES] = TEX_MAINMENU_TEXT_CONFIGURACION,
                   [LANG_EN] = TEX_MAINMENU_TEXT_CONFIGURATION,
                   [LANG_EU] = TEX_MAINMENU_TEXT_KONFIGURAZIOA}},

        [2] = {{576.0f, 550.0f, 520.0f, 100.0f}, true, {
                   [LANG_ES] = TEX_MAINMENU_TEXT_VOLVER_AL_MENU,
                   [LANG_EN] = TEX_MAINMENU_TEXT_MENU,
                   [LANG_EU] = TEX_MAINMENU_TEXT_ITZULI_MENURA}},
    };

    //Mouse to canvas space
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

    //TODO: comes from the translation system
    Language current_language = LANG_ES;

    for (int i = 0; i < (int)(sizeof(UITexts) / sizeof(UITexts[0])); i++) {
        const CanvasElementText *label = &UITexts[i];

        TextureId label_id = label->translated ? label->textures[current_language]
                                               : label->textures[LANG_ES];
        TextureAsset asset = texture_get(label_id);

        //Fixed height, width from the texture's ratio
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

    //Click handling. Careful: indices must match the table above
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse_canvas, UIElements[0].area)) {
            resume_game();
        } else if (CheckCollisionPointRec(mouse_canvas, UIElements[1].area)) {
            //CONFIG: nothing yet
        } else if (CheckCollisionPointRec(mouse_canvas, UIElements[2].area)) {
            exit_to_menu();
        }
    }
}

void game_load_draw(void (*set_menu)(void)){
    static float elapsed = 0.0f;
    elapsed += GetFrameTime();

    float p = elapsed / LOADING_DURATION;
    if (p > 1.0f) p = 1.0f;

    //Smoothstep: starts and ends slowly
    float eased = p * p * (3.0f - 2.0f * p);

    //Alpha runs ahead of the sorting, so it finishes appearing first
    float alpha = eased * 1.15f;
    if (alpha > 1.0f) alpha = 1.0f;

    float amount = (1.0f - eased) * LOADING_MAX_OFFSET;

    float scale_x = (float)GetScreenWidth()  / CANVAS_WIDTH;
    float scale_y = (float)GetScreenHeight() / CANVAS_HEIGHT;
    float scale   = (scale_x < scale_y) ? scale_x : scale_y;

    float offset_x = (GetScreenWidth()  - CANVAS_WIDTH * scale) / 2.0f;
    float offset_y = (GetScreenHeight() - CANVAS_HEIGHT * scale) / 2.0f;

    //Logo big and centered here. It snaps to its menu spot on the swap
    static const Rectangle bg_area   = {  0.0f,   0.0f, 1672.0f, 941.0f};
    static const Rectangle logo_area = {386.0f, 179.0f,  900.0f, 583.0f};

    draw_glitched(TEX_MAINMENU_BG,   bg_area,   alpha, amount, scale, offset_x, offset_y);
    draw_glitched(TEX_MAINMENU_LOGO, logo_area, alpha, amount, scale, offset_x, offset_y);

    if (elapsed >= LOADING_DURATION) {
        elapsed = 0.0f;
        set_menu();
    }
}

void game_draw(const Entity *entities, int count, bool debug, Camera2D *camera, const Map *map){
    BeginMode2D(*camera);

    map_draw(map, 0, map->layer_cut);
    
    //Draw the player first
    if(entities[0].active){
        entity_draw(&entities[0]);
    }

    for (int i = 1; i < count; i++) {
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