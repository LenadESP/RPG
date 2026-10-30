#include "entity.h"
#include "game.h"
#include "raylib.h"
#include "collision.h"
#include "map.h"

void game_menu_draw(void){
    int size = 40;
    const char *txt = "Press ENTER to start!";
    DrawText(txt, GetScreenWidth()/2 - MeasureText(txt, size)/2, 500, size, BLACK);
}

void game_pause_draw(const Entity *entities, int count, bool debug, Camera2D *camera, Map *map){
    game_draw(entities, count, debug, camera, map);

    int size = 40;
    const char *txt = "Paused! Press ESC to resume";
    DrawText(txt, GetScreenWidth()/2 - MeasureText(txt, size)/2, 500, size, BLACK);    
}

void game_draw(const Entity *entities, int count, bool debug, Camera2D *camera, Map *map){
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