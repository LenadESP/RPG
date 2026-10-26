#include "entity.h"
#include "screen.h"
#include "raylib.h"
#include "collision.h"

void screen_menu_draw(void){
    int size = 40;
    const char *txt = "Press ENTER to start!";
    DrawText(txt, GetScreenWidth()/2 - MeasureText(txt, size)/2, 500, size, BLACK);
}

void screen_pause_draw(const Entity *entities, int count, bool debug, Camera2D *camera){
    screen_game_draw(entities, count, debug, camera);

    int size = 40;
    const char *txt = "Paused! Press ESC to resume";
    DrawText(txt, GetScreenWidth()/2 - MeasureText(txt, size)/2, 500, size, BLACK);    
}

void screen_game_draw(const Entity *entities, int count, bool debug, Camera2D *camera){
    BeginMode2D(*camera);
    
    for (int i = 0; i < count; i++) {
        if (!entities[i].active) continue;

        entity_draw(&entities[i]);
        if(debug){
            DrawRectangleLinesEx(collision_rect(&entities[i]), 1, BLACK);
        }
    }

    EndMode2D();
}