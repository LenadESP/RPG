#include "entity.h"
#include "game.h"
#include "raylib.h"
#include "collision.h"

void game_menu_draw(void){
    int size = 40;
    const char *txt = "Press ENTER to start!";
    DrawText(txt, GetScreenWidth()/2 - MeasureText(txt, size)/2, 500, size, BLACK);
}

void game_pause_draw(const Entity *entities, int count, bool debug, Camera2D *camera){
    game_draw(entities, count, debug, camera);

    int size = 40;
    const char *txt = "Paused! Press ESC to resume";
    DrawText(txt, GetScreenWidth()/2 - MeasureText(txt, size)/2, 500, size, BLACK);    
}

void game_draw(const Entity *entities, int count, bool debug, Camera2D *camera){
    BeginMode2D(*camera);


    //PLACEHOLDER: Reference grid before actual tilemap.
    for (int x = -2000; x <= 2000; x += 16) {
        DrawLine(x, -2000, x, 2000, LIGHTGRAY);
    }
    for (int y = -2000; y <= 2000; y += 16) {
        DrawLine(-2000, y, 2000, y, LIGHTGRAY);
    }
    //ENDOF PLACEHOLDER

    
    for (int i = 0; i < count; i++) {
        if (!entities[i].active) continue;

        entity_draw(&entities[i]);

        //Drawing the hitbox
        if(debug){
            DrawRectangleLinesEx(collision_rect(&entities[i]), 1, YELLOW);
        }
    }

    EndMode2D();
}