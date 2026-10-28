#ifndef GAME_H
#define GAME_H

#include "entity.h"
#include "raylib.h"

void game_menu_draw(void);
void game_pause_draw(const Entity *entities, int count, bool debug, Camera2D *camera);
void game_draw(const Entity *entities, int count, bool debug, Camera2D *camera);

#endif