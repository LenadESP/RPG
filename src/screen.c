#include "entity.h"
#include "screen.h"
#include "raylib.h"

void screen_menu_draw(void){
    int size = 40;
    const char *txt = "Press ENTER to start!";
    DrawText(txt, GetScreenWidth()/2 - MeasureText(txt, size)/2, 500, size, BLACK);
}

void screen_pause_draw(const Entity *entities, int count){
    screen_game_draw(entities, count);

    int size = 40;
    const char *txt = "Paused! Press ESC to resume";
    DrawText(txt, GetScreenWidth()/2 - MeasureText(txt, size)/2, 500, size, BLACK);    
}

void screen_game_draw(const Entity *entities, int count){
    for (int i = 0; i < count; i++) {
        if (!entities[i].active) continue;
        entity_draw(entities[i]);
    }
}