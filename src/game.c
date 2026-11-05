#include "entity.h"
#include "game.h"
#include "raylib.h"
#include "collision.h"
#include "map.h"
#include "defines.h"
#include "texture.h"

void game_menu_draw(void){
    //holy hardcoding. I'm done with good architecture looooooool
    float scale_x = (float)GetScreenWidth()  / CANVAS_W;
    float scale_y = (float)GetScreenHeight() / CANVAS_H;
    float scale   = (scale_x < scale_y) ? scale_x : scale_y; //operador ternario, elige el menor.

    float offset_x = (GetScreenWidth()  - CANVAS_W * scale) / 2.0f;
    float offset_y = (GetScreenHeight() - CANVAS_H * scale) / 2.0f;

    TextureAsset bg = texture_get(TEX_MAINMENU_BG);
    TextureAsset logo = texture_get(TEX_MAINMENU_LOGO);

    DrawTexturePro(
    bg.texture, 
    (Rectangle){
        0,
        0,
        bg.frame_w,
        bg.frame_h
    }, 
    (Rectangle){
        offset_x + 0 * scale,
        offset_y + 0 * scale,
        bg.frame_w * scale,
        bg.frame_h * scale
    },     
    (Vector2){
        0, 0
    }, 0, WHITE);

    DrawTexturePro(
    logo.texture, 
    (Rectangle){
        0,
        0,
        logo.frame_w,
        logo.frame_h
    }, 
    (Rectangle){
        offset_x + 523.5 * scale,
        offset_y + 60 * scale,
        625 * scale,
        405 * scale
    },     
    (Vector2){
        0, 0
    }, 0, WHITE);
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