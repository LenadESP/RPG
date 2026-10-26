#ifndef SCREEN_H
#define SCREEN_H

#include "entity.h"
#include "raylib.h"

void screen_menu_draw(void);
void screen_pause_draw(const Entity *entities, int count, bool debug, Camera2D *camera);
void screen_game_draw(const Entity *entities, int count, bool debug, Camera2D *camera);

#endif