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
    screen_game_draw(entities, count, debug);

    int size = 40;
    const char *txt = "Paused! Press ESC to resume";
    DrawText(txt, GetScreenWidth()/2 - MeasureText(txt, size)/2, 500, size, BLACK);    
}

void screen_game_draw(const Entity *entities, int count, bool debug, Camera2D *camera){
    camera = {
        (Vector2) {0,0}, //offset
        (Vector2) {entities[0].x + entities[0].size / 2.0f, entities[0].y + entities[0]size / 2.0f}, //middle of player
        0.0f,
        1.0f
    }

    void BeginMode2D(camera);
    
    for (int i = 0; i < count; i++) {
        if (!entities[i].active) continue;

        entity_draw(&entities[i]);
        if(debug){
            DrawRectangleLinesEx(collision_rect(&entities[i]), 1, BLACK);
        }
    }

    void EndMode2D(void);
}