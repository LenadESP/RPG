#ifndef GAME_H
#define GAME_H

#include "entity.h"
#include "raylib.h"
#include "map.h"

void game_menu_draw(void (*start_game)(void), void (*close_game)(void));
void game_pause_draw(const Entity *entities, int count, bool debug, Camera2D *camera, const Map *map, void (*resume_game)(void), void (*exit_to_menu)(void));
void game_draw(const Entity *entities, int count, bool debug, Camera2D *camera, const Map *map);

#endif